#include<stdio.h>
#include<conio.h>
#include<iostream>
#include<fstream>
#include<unordered_map>
#include<unordered_set>
#include<vector>
#include<sstream>
#include<queue>
#include<set>
#include<stack>
#include <algorithm>
using namespace std;
unordered_map<string, int> keywords; // for inbuilt keywords
unordered_map<string, struct node*> code; // maps line number to node
unordered_map<string, vector<struct node*>> variable_name_to_nodes; // for to find out all definition node of variable
// temp_variable_name_to_nodes_defined, temp_variable_name_to_nodes_used; 
unordered_map<string, struct node*> function_map;
unordered_map<struct node*, struct node*> return_statements;
unordered_set<string> datatypes; // for inbuilt datatypes
vector<unordered_map<string, vector<struct node*>>> threads_variable_name_to_nodes_defined, threads_variable_name_to_nodes_used; // different scope for different threads
vector < stack<struct node*> > thread_control_dependence; // in case of parallel computation to keep track of scopes for different threads
unordered_set<string> shared_variables; // to keep track of shared variables between threads
unordered_set<string> thread_private_variables; // to keep track of thread_private variables
stack<struct node*> control_dependence; // to keep track of scope for main thread for without parallel compution code
stack<string> control_dependence2; // for to create trace keep track of scope
string file_opening_line = "ofstream result;\nresult.open(\"result.txt\", ios::app);\nresult<<";
string for_main_file_opening_line = "ofstream result;\nresult.open(\"result.txt\", ios::app);\nresult<<";
vector<string> source_code; // to find out line by line_number directly
vector<int> conditional_statements; // to keep track of hierarchy of if else statements
bool while_loop_flag = 0, for_loop_flag = 0, function_starting = 0, condition_flag = 0;
int in_parallel_flag = 0; // to check current line is under parallel computing
int count_for_sections = 0; // for to check when closing bracket of sections construct comes
bool has_sections = 0; // to mark that code contains sections construct
unordered_map<string, vector<string>> marking_lines; // for dependent lines, mark lines which flags dependent lines to include
unordered_map<string, vector<int>> dependent_lines; // lines like sections and for which haven't processed but should include if marked lines are added
bool has_critical = 0, has_atomic = 0;
bool functions_added = 0;
int brackets = 0;
ofstream temp_source;
int total_number_of_lines_of_header_files = 0;
unordered_set<string> visited_lines;

void process(string line, string number);

struct node {
    unordered_set<struct node*> parent; // contains control, interference and data dependences
    vector<struct node*> parallel_thread_nodes; // after completing parallel region it contains refernce to nodes with same name and last definition from another thread
    struct node* parametere_in_edge, * parametere_out_edge, * calling_edge; // parameter IN/OUT edge
    struct node* return_link;
    vector<struct node*> transitive_edge, affect_return_edge, interference_edge;
    bool mark; // for dynamic dependence graph
    bool flag; // for transitive edge and affect return edge
    bool in_thread; // for to create different variable scope for different threads
    string line_number;
    string statement;
    unordered_set<string> used, defined; // keeps track of used and defined nodes on that line
    vector<struct node*> formal_in_nodes, formal_out_nodes, actual_in_nodes, actual_out_nodes; // nodes used for inter function slicing

    node(string number, string line) {
        parametere_out_edge = NULL;
        parametere_in_edge = NULL;
        calling_edge = NULL;
        return_link = NULL;

        flag = false;
        mark = false;
        in_thread = false;
        line_number = number;
        statement = line;
    }

};

void keywords_init() {
    keywords["for"] = keywords.size();
    keywords["while"] = keywords.size();
    keywords["if"] = keywords.size();
    keywords["else"] = keywords.size();
    keywords["cin"] = keywords.size();
    keywords["cout"] = keywords.size();
    keywords["return"] = keywords.size();
    keywords["pragma"] = keywords.size();

    datatypes.insert("int");
    datatypes.insert("float");
    datatypes.insert("double");
    datatypes.insert("string");
    datatypes.insert("void");
    datatypes.insert("static");
}

//parse words from the line
pair<int, string> find_token(int i, string line) {
    string temp = "";
    bool is_string = 0;
    while (i < line.length() && !(line[i] >= 97 && line[i] <= 122) && !(line[i] >= 65 && line[i] <= 90)
        && !(line[i] >= 48 && line[i] <= 57) && line[i] != '_' && line[i] != '"' && line[i] != '\'') {
        i++;
    }

    if (i < line.length() && line[i] == '"') {
        i++;
        while (i < line.length() && line[i] != '"') {
            i++;
        }
        i++;
        return find_token(i, line);
    }

    while (i < line.length() && ((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90)
        || (line[i] >= 48 && line[i] <= 57) || line[i] == '_' || line[i] == '"' || line[i] == '\'')) {
        temp += line[i++];
    }

    if (datatypes.find(temp) != datatypes.end()) {
        return find_token(i, line);
    }
    return { i, temp };
}

// find defined and used variables and create node for the statement and push node into code map for defined variable
void process_definition(string line, int currentIndex, string number) {
    int thread_number;
    if (in_parallel_flag) {
        stringstream s1(number);
        string t1;
        getline(s1, t1, '@');
        getline(s1, t1, '@');
        thread_number = stoi(t1);
    }

    int i = currentIndex;
    string line1 = line;
    line = line.substr(i);
    vector<string> tokens;
    string temp;
    stringstream temp_line(line);
    while (getline(temp_line, temp, '=')) {
        tokens.push_back(temp);
    }
    if (tokens.size() < 2) {
        return;
    }

    struct node* temp_node;
    if (code.find(number) == code.end()) {
        temp_node = new node(number, line1);
    }
    else {
        temp_node = code[number];
    }

    string variable_name;
    while (i < tokens[0].length()) {
        pair<int, string> token = find_token(i, tokens[0]);
        i = token.first;
        if (datatypes.find(token.second) == datatypes.end() && token.second.length() > 0)
            variable_name = token.second;
    }

    (temp_node->defined).insert(variable_name);

    i = 0;
    line = tokens[1];

    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;
        if (in_parallel_flag && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {

            while (threads_variable_name_to_nodes_used.size() < thread_number + 1) {
                unordered_map < string, vector<struct node*>> mp;
                threads_variable_name_to_nodes_used.push_back(mp);
            }
            threads_variable_name_to_nodes_used[thread_number][temp].push_back(temp_node);
        }

        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && threads_variable_name_to_nodes_defined.size() > thread_number &&
            threads_variable_name_to_nodes_defined[thread_number].find(temp) == threads_variable_name_to_nodes_defined[thread_number].end()) {
            cout << "The variable name " << temp << " does not exist in definition portion\n";
        }
        else {
            (temp_node->used).insert(temp);
            vector<struct node*> v;
            if (in_parallel_flag && threads_variable_name_to_nodes_defined.size() > thread_number && threads_variable_name_to_nodes_defined[thread_number].find(temp) != threads_variable_name_to_nodes_defined[thread_number].end()) {
                v = threads_variable_name_to_nodes_defined[thread_number][temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            temp_node->parent.insert(v[v.size() - 1]);
        }
    }
    if (!control_dependence.empty()) {
        struct node* parent = control_dependence.top();
        if (in_parallel_flag) {
            stringstream ss1(number);
            string temp1;
            getline(ss1, temp1, '@');
            getline(ss1, temp1, '@');
            int num = stoi(temp1);
            if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
                parent = thread_control_dependence[num].top();
            }
        }
        temp_node->parent.insert(parent);
    }
    if (in_parallel_flag) {
        while (threads_variable_name_to_nodes_defined.size() < thread_number + 1) {
            unordered_map < string, vector<struct node*>> mp;
            threads_variable_name_to_nodes_defined.push_back(mp);
        }

        threads_variable_name_to_nodes_defined[thread_number][variable_name].push_back(temp_node);
    }
    else {
        variable_name_to_nodes[variable_name].push_back(temp_node);
    }
    code[number] = temp_node;
}

void process_for(string line, int currentIndex, string number) {
    stringstream s1(number);
    string t1;
    getline(s1, t1, '@');
    getline(s1, t1, '@');
    int thread_number = stoi(t1);
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    code[number] = temp_node;
    string temp1;
    vector<string> tokenized_line;
    vector<string> tokenized_number;
    stringstream temp1_line(line.substr(currentIndex + 1));
    while (getline(temp1_line, temp1, ';')) {
        tokenized_line.push_back(temp1);
    }
    stringstream temp2_line(number);
    getline(temp2_line, temp1, '_');
    tokenized_number.push_back(temp1);
    getline(temp2_line, temp1, '@');
    tokenized_number.push_back(temp1);

    // add line into marking lines
    string eralier_line = source_code[stoi(tokenized_number[0]) - 1];
    pair<int, string> eralier_p = find_token(0, eralier_line);
    eralier_p = find_token(eralier_p.first, eralier_line);
    eralier_p = find_token(eralier_p.first, eralier_line);
    if (eralier_p.second == "for") {
        marking_lines["for"].push_back(number);
    }

    if (tokenized_number[1] == "0") {
        process_definition(tokenized_line[0], 0, number);
    }
    else {
        process_definition(tokenized_line[2], 0, number);
    }

    i = 0;
    while (i < tokenized_line[1].length()) {
        pair<int, string> token = find_token(i, tokenized_line[1]);

        i = token.first;
        string temp = token.second;
        if (in_parallel_flag && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {
            while (threads_variable_name_to_nodes_used.size() < thread_number + 1) {
                unordered_map < string, vector<struct node*>> mp;
                threads_variable_name_to_nodes_used.push_back(mp);
            }
            threads_variable_name_to_nodes_used[thread_number][temp].push_back(temp_node);
        }
        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && threads_variable_name_to_nodes_defined.size() > thread_number &&
            threads_variable_name_to_nodes_defined[thread_number].find(temp) == threads_variable_name_to_nodes_defined[thread_number].end()) {
            cout << "The variable name " << temp << " does not exist in if portion\n";
        }
        else {
            (temp_node->used).insert(temp);
            vector<struct node*> v;
            if (threads_variable_name_to_nodes_defined.size() > thread_number && threads_variable_name_to_nodes_defined[thread_number].find(temp) != threads_variable_name_to_nodes_defined[thread_number].end()) {
                v = threads_variable_name_to_nodes_defined[thread_number][temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            temp_node->parent.insert(v[v.size() - 1]);
        }
    }
}

void process_while(string line, int currentIndex, string number) {
    stringstream s1(number);
    string t1;
    getline(s1, t1, '@');
    getline(s1, t1, '@');
    int thread_number = stoi(t1);
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;
        if (in_parallel_flag && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {
            while (threads_variable_name_to_nodes_used.size() < thread_number + 1) {
                unordered_map < string, vector<struct node*>> mp;
                threads_variable_name_to_nodes_used.push_back(mp);
            }
            threads_variable_name_to_nodes_used[thread_number][temp].push_back(temp_node);
        }
        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && threads_variable_name_to_nodes_defined.size() > thread_number &&
            threads_variable_name_to_nodes_defined[thread_number].find(temp) == threads_variable_name_to_nodes_defined[thread_number].end()) {
            cout << "The variable name " << temp << " does not exist in if portion\n";
        }
        else {
            (temp_node->used).insert(temp);
            vector<struct node*> v;
            if (threads_variable_name_to_nodes_defined.size() > thread_number && threads_variable_name_to_nodes_defined[thread_number].find(temp) != threads_variable_name_to_nodes_defined[thread_number].end()) {
                v = threads_variable_name_to_nodes_defined[thread_number][temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            temp_node->parent.insert(v[v.size() - 1]);
        }
    }
    code[number] = temp_node;
}

void process_if(string line, int currentIndex, string number) {
    stringstream s1(number);
    string t1;
    getline(s1, t1, '@');
    getline(s1, t1, '@');
    int thread_number = stoi(t1);
    int i = currentIndex;
    struct node* temp_node;
    if (code.find(number) != code.end()) {
        temp_node = code[number];
    }
    else {
        temp_node = new node(number, line);
    }
    struct node* parent = control_dependence.top();
    if (in_parallel_flag) {
        stringstream ss1(number);
        string temp1;
        getline(ss1, temp1, '@');
        getline(ss1, temp1, '@');
        int num = stoi(temp1);
        if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
            parent = thread_control_dependence[num].top();
        }
    }
    temp_node->parent.insert(parent);
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;
        if (in_parallel_flag && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {
            while (threads_variable_name_to_nodes_used.size() < thread_number + 1) {
                unordered_map < string, vector<struct node*>> mp;
                threads_variable_name_to_nodes_used.push_back(mp);
            }
            threads_variable_name_to_nodes_used[thread_number][temp].push_back(temp_node);
        }
        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && threads_variable_name_to_nodes_defined.size() > thread_number &&
            threads_variable_name_to_nodes_defined[thread_number].find(temp) == threads_variable_name_to_nodes_defined[thread_number].end()) {
            cout << "The variable name " << temp << " does not exist in if portion\n";
        }
        else {
            (temp_node->used).insert(temp);
            vector<struct node*> v;
            if (threads_variable_name_to_nodes_defined.size() > thread_number && threads_variable_name_to_nodes_defined[thread_number].find(temp) != threads_variable_name_to_nodes_defined[thread_number].end()) {
                v = threads_variable_name_to_nodes_defined[thread_number][temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            temp_node->parent.insert(v[v.size() - 1]);
        }
    }
    code[number] = temp_node;
}

void process_else(string line, int currentIndex, string number) {
    int i = currentIndex;
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;

        if (temp == "if") {
            struct node* temp_node = new node(number, line);
            code[number] = temp_node;
            process_if(line, i, number);
            return;
        }
    }
    struct node* temp_node = new node(number, line);
    if (!control_dependence.empty()) {
        struct node* parent = control_dependence.top();
        if (in_parallel_flag) {
            stringstream ss1(number);
            string temp1;
            getline(ss1, temp1, '@');
            getline(ss1, temp1, '@');
            int num = stoi(temp1);
            if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
                parent = thread_control_dependence[num].top();
            }
        }
        temp_node->parent.insert(parent);
    }
    code[number] = temp_node;
}

void process_cin(string line, int currentIndex, string number) {
    stringstream s1(number);
    string t1;
    getline(s1, t1, '@');
    getline(s1, t1, '@');
    int thread_number = stoi(t1);
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;
        if (temp == "") {
            continue;
        }
        (temp_node->defined).insert(temp);
        vector<struct node*> v;
        if (in_parallel_flag) {
            while (threads_variable_name_to_nodes_defined.size() < thread_number + 1) {
                unordered_map < string, vector<struct node*>> mp;
                threads_variable_name_to_nodes_defined.push_back(mp);
            }
            threads_variable_name_to_nodes_defined[thread_number][temp].push_back(temp_node);
        }
        else {
            variable_name_to_nodes[temp].push_back(temp_node);
        }
    }
    if (!control_dependence.empty()) {
        struct node* parent = control_dependence.top();
        if (in_parallel_flag) {
            stringstream ss1(number);
            string temp1;
            getline(ss1, temp1, '@');
            getline(ss1, temp1, '@');
            int num = stoi(temp1);
            if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
                parent = thread_control_dependence[num].top();
            }
        }
        temp_node->parent.insert(parent);
    }
    code[number] = temp_node;
}

//not cosidered case of "variable_name"
void process_cout(string line, int currentIndex, string number) {
    stringstream s1(number);
    string t1;
    getline(s1, t1, '@');
    getline(s1, t1, '@');
    int thread_number = stoi(t1);
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;
        if (in_parallel_flag && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {
            while (threads_variable_name_to_nodes_used.size() < thread_number + 1) {
                unordered_map < string, vector<struct node*>> mp;
                threads_variable_name_to_nodes_used.push_back(mp);
            }
            threads_variable_name_to_nodes_used[thread_number][temp].push_back(temp_node);
        }
        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'' || temp == "endl") {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && threads_variable_name_to_nodes_defined.size() > thread_number &&
            threads_variable_name_to_nodes_defined[thread_number].find(temp) == threads_variable_name_to_nodes_defined[thread_number].end()) {
            cout << number << " : The variable name " << temp << " does not exist in cout portion\n";
        }
        else {
            vector<struct node*> v;
            if (threads_variable_name_to_nodes_defined.size() > thread_number && threads_variable_name_to_nodes_defined[thread_number].find(temp) != threads_variable_name_to_nodes_defined[thread_number].end()) {
                v = threads_variable_name_to_nodes_defined[thread_number][temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            (temp_node->used).insert(temp);
            temp_node->parent.insert(v[v.size() - 1]);
        }
    }
    if (!control_dependence.empty()) {
        struct node* parent = control_dependence.top();
        if (in_parallel_flag) {
            stringstream ss1(number);
            string temp1;
            getline(ss1, temp1, '@');
            getline(ss1, temp1, '@');
            int num = stoi(temp1);
            if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
                parent = thread_control_dependence[num].top();
            }
        }
        temp_node->parent.insert(parent);
    }
    code[number] = temp_node;
}

void find_transitive_dependence_edge(struct node* function_calling_node, string function_name) {
    vector<struct node*> v = function_map[function_name]->formal_out_nodes;
    for (int i = 0; i < v.size(); i++) {
        queue<struct node*> q;

        // to mark visited nodes
        q.push(v[i]);
        while (!q.empty()) {
            struct node* temp_node = q.front();
            temp_node->flag = true;
            q.pop();
            unordered_set<struct node*> v1 = temp_node->parent;
            for (auto it : v1) {
                if (!it->flag) {
                    q.push(it);
                }
            }
        }

        vector<struct node*> v2 = function_map[function_name]->formal_in_nodes;
        for (int j = 0; j < v2.size(); j++) {
            if (v2[j]->flag) {
                function_calling_node->actual_out_nodes[i]->transitive_edge.push_back(function_calling_node->actual_in_nodes[j]);
            }
        }

        // to unmark visited nodes
        q.push(v[i]);
        while (!q.empty()) {
            struct node* temp_node = q.front();
            temp_node->flag = false;
            q.pop();
            unordered_set<struct node*> v1 = temp_node->parent;
            for (auto it : v1) {
                if (it->flag) {
                    q.push(it);
                }
            }
        }
    }
}

void find_affect_return_edge(struct node* function_calling_node, string function_name) {

    struct node* return_node = return_statements[function_map[function_name]];

    queue<struct node*> q;
    // to mark visited nodes
    q.push(return_node);
    while (!q.empty()) {
        struct node* temp_node = q.front();
        temp_node->flag = true;
        q.pop();
        unordered_set<struct node*> v1 = temp_node->parent;
        for (auto it : v1) {
            if (!it->flag) {
                q.push(it);
            }
        }
    }

    vector<struct node*> v2 = function_map[function_name]->formal_in_nodes;
    for (int j = 0; j < v2.size(); j++) {
        if (v2[j]->flag) {
            function_calling_node->affect_return_edge.push_back(function_calling_node->actual_in_nodes[j]);
        }
    }

    // to unmark visited nodes
    q.push(return_node);
    while (!q.empty()) {
        struct node* temp_node = q.front();
        temp_node->flag = false;
        q.pop();
        unordered_set<struct node*> v1 = temp_node->parent;
        for (auto it : v1) {
            if (it->flag) {
                q.push(it);
            }
        }
    }
}

void process_function_call(string line, string number, int currentIndex, bool return_value_expects) {
    stringstream s1(number);
    string t1;
    getline(s1, t1, '@');
    getline(s1, t1, '@');
    int thread_number = stoi(t1);
    struct node* temp_node = new node(number, line); // node of function

    int i = 0;
    if (return_value_expects) {
        while (i < line.length() && line[i] != '=') {
            i++;
        }
    }
    // to find name of function
    pair<int, string> p = find_token(i, line);
    string function_name = p.second;

    // in case of library function call
    if (function_map.find(function_name) == function_map.end()) {
        //if return_value_expects than find which variable is defined
        if (return_value_expects) {
            int i = 0;
            while (i < line.length() && line[i] != '=') {
                i++;
            }
            while (i >= 0 && !((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90))) {
                i--;
            }
            string temp = "";
            while (i >= 0 && ((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90))) {
                temp = line[i--] + temp;
            }
            temp_node->defined.insert(temp);
            if (in_parallel_flag) {
                while (threads_variable_name_to_nodes_defined.size() < thread_number + 1) {
                    unordered_map < string, vector<struct node*>> mp;
                    threads_variable_name_to_nodes_defined.push_back(mp);
                }
                threads_variable_name_to_nodes_defined[thread_number][temp].push_back(temp_node);
            }
            else {
                variable_name_to_nodes[temp].push_back(temp_node);
            }
        }

        i = currentIndex;
        while (i < line.length()) {
            pair<int, string> token = find_token(i, line);

            i = token.first;
            string temp = token.second;

            if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
                continue;
            }
            else {
                // data dependence
                vector<struct node*> v;
                if (threads_variable_name_to_nodes_defined.size() > thread_number && threads_variable_name_to_nodes_defined[thread_number].find(temp) != threads_variable_name_to_nodes_defined[thread_number].end()) {
                    v = threads_variable_name_to_nodes_defined[thread_number][temp];
                }
                else {
                    v = variable_name_to_nodes[temp];
                }
                temp_node->parent.insert(v[v.size() - 1]);
            }
        }
        code[number] = temp_node;
        return;
    }

    i = currentIndex;
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;

        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        }
        else {
            // data dependence
            vector<struct node*> v;
            if (threads_variable_name_to_nodes_defined.size() > thread_number && threads_variable_name_to_nodes_defined[thread_number].find(temp) != threads_variable_name_to_nodes_defined[thread_number].end()) {
                v = threads_variable_name_to_nodes_defined[thread_number][temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            temp_node->parent.insert(v[v.size() - 1]);

            string temp1 = temp + "_in";
            struct node* actual_in_node = new node(number, temp1 + " = " + temp);
            actual_in_node->parent.insert(temp_node);

            if (in_parallel_flag) {
                while (threads_variable_name_to_nodes_defined.size() < thread_number + 1) {
                    unordered_map < string, vector<struct node*>> mp;
                    threads_variable_name_to_nodes_defined.push_back(mp);
                }
                threads_variable_name_to_nodes_defined[thread_number][temp1].push_back(actual_in_node);
                while (threads_variable_name_to_nodes_used.size() < thread_number + 1) {
                    unordered_map < string, vector<struct node*>> mp;
                    threads_variable_name_to_nodes_used.push_back(mp);
                }
                threads_variable_name_to_nodes_used[thread_number][temp].push_back(temp_node);
            }
            else {
                variable_name_to_nodes[temp1].push_back(actual_in_node);
            }
            (actual_in_node->used).insert(temp);
            (actual_in_node->defined).insert(temp1);
            temp_node->actual_in_nodes.push_back(actual_in_node);
            actual_in_node->parent.insert(temp_node);
            if (i - temp.length() - 1 >= 0 && line[i - temp.length() - 1] == '&') {
                temp1 = temp + "_out";
                struct node* actual_out_node = new node(number, temp + " = &" + temp1);
                actual_out_node->parent.insert(temp_node);

                if (in_parallel_flag) {
                    while (threads_variable_name_to_nodes_defined.size() < thread_number + 1) {
                        unordered_map < string, vector<struct node*>> mp;
                        threads_variable_name_to_nodes_defined.push_back(mp);
                    }
                    threads_variable_name_to_nodes_defined[thread_number][temp].push_back(actual_out_node);
                    while (threads_variable_name_to_nodes_used.size() < thread_number + 1) {
                        unordered_map < string, vector<struct node*>> mp;
                        threads_variable_name_to_nodes_used.push_back(mp);
                    }
                    threads_variable_name_to_nodes_used[thread_number][temp1].push_back(temp_node);
                }
                else {
                    variable_name_to_nodes[temp].push_back(actual_out_node);
                }

                (actual_out_node->used).insert(temp1);
                (actual_out_node->defined).insert(temp);
                temp_node->actual_out_nodes.push_back(actual_out_node);
                actual_out_node->parent.insert(temp_node);
            }
        }
    }

    // for parameter edge
    if (temp_node->actual_in_nodes.size() > 0) {
        vector<struct node*> v1 = function_map[function_name]->formal_in_nodes;
        vector<struct node*> v2 = temp_node->actual_in_nodes;
        for (int i = 0; i < v1.size(); i++) {
            v1[i]->parametere_in_edge = v2[i];
        }
        v1 = function_map[function_name]->formal_out_nodes;
        v2 = temp_node->actual_out_nodes;
        for (int i = 0; i < v1.size(); i++) {
            v2[i]->parametere_out_edge = v1[i];
        }
    }

    // for calling edge
    function_map[function_name]->calling_edge = temp_node;

    // for transitive edge
    if (temp_node->actual_out_nodes.size() > 0) {
        find_transitive_dependence_edge(temp_node, function_name);
    }

    //return link edge
    if (return_value_expects && return_statements.find(function_map[function_name]) != return_statements.end()) {
        struct node* return_node = return_statements[function_map[function_name]];
        temp_node->return_link = return_node;
    }

    //affect return edge
    if (return_value_expects && temp_node->actual_in_nodes.size() > 0) {
        find_affect_return_edge(temp_node, function_name);
    }

    //if return_value_expects than find which variable is defined
    if (return_value_expects) {
        int i = 0;
        while (i < line.length() && line[i] != '=') {
            i++;
        }
        while (i >= 0 && !((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90))) {
            i--;
        }
        string temp = "";
        while (i >= 0 && ((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90))) {
            temp = line[i--] + temp;
        }
        temp_node->defined.insert(temp);
        if (in_parallel_flag) {
            while (threads_variable_name_to_nodes_defined.size() < thread_number + 1) {
                unordered_map < string, vector<struct node*>> mp;
                threads_variable_name_to_nodes_defined.push_back(mp);
            }
            threads_variable_name_to_nodes_defined[thread_number][temp].push_back(temp_node);
        }
        else {
            variable_name_to_nodes[temp].push_back(temp_node);
        }
    }

    //control dependence
    if (!control_dependence.empty()) {
        struct node* parent = control_dependence.top();
        if (in_parallel_flag) {
            stringstream ss1(number);
            string temp1;
            getline(ss1, temp1, '@');
            getline(ss1, temp1, '@');
            int num = stoi(temp1);
            if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
                parent = thread_control_dependence[num].top();
            }
        }
        temp_node->parent.insert(parent);
    }
    code[number] = temp_node;
}

void process_function_definition(string line, string number, int currentIndex) {
    variable_name_to_nodes.clear();                        /////////      ********       Alert!!!!!!!!
    struct node* temp_node = new node(number, line); // node of function
    int i = currentIndex;
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;

        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'' || datatypes.find(temp) != datatypes.end()) {
            continue;
        }
        else {
            string temp1 = temp + "_in";
            struct node* formal_in_node = new node(number, temp + " = " + temp1);
            formal_in_node->parent.insert(temp_node);
            variable_name_to_nodes[temp].push_back(formal_in_node);

            (formal_in_node->used).insert(temp1);
            (formal_in_node->defined).insert(temp);
            temp_node->formal_in_nodes.push_back(formal_in_node);
            formal_in_node->parent.insert(temp_node);
            if (i - temp.length() - 1 >= 0 && line[i - temp.length() - 1] == '*') {
                temp1 = temp + "_out";
                struct node* formal_out_node = new node(number, temp1 + " = *" + temp);
                formal_out_node->parent.insert(temp_node);

                variable_name_to_nodes[temp1].push_back(formal_out_node);

                (formal_out_node->used).insert(temp1);
                (formal_out_node->defined).insert(temp1);
                temp_node->formal_out_nodes.push_back(formal_out_node);
                formal_out_node->parent.insert(temp_node);
            }
        }
    }

    // to find name of function
    pair<int, string> p = find_token(0, line);
    function_map[p.second] = temp_node;
    code[number] = temp_node;
}

void process_return(string line, int currentIndex, string number) {
    stringstream s1(number);
    string t1;
    getline(s1, t1, '@');
    getline(s1, t1, '@');
    int thread_number = stoi(t1);
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;
        if (in_parallel_flag && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {
            while (threads_variable_name_to_nodes_used.size() < thread_number + 1) {
                unordered_map < string, vector<struct node*>> mp;
                threads_variable_name_to_nodes_used.push_back(mp);
            }
            threads_variable_name_to_nodes_used[thread_number][temp].push_back(temp_node);
        }
        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'' || temp == "endl") {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && threads_variable_name_to_nodes_defined.size() > thread_number &&
            threads_variable_name_to_nodes_defined[thread_number].find(temp) == threads_variable_name_to_nodes_defined[thread_number].end()) {
            cout << "The variable name " << temp << " does not exist in cout portion\n";
        }
        else {
            vector<struct node*> v;
            if (threads_variable_name_to_nodes_defined.size() > thread_number && threads_variable_name_to_nodes_defined[thread_number].find(temp) != threads_variable_name_to_nodes_defined[thread_number].end()) {
                v = threads_variable_name_to_nodes_defined[thread_number][temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            (temp_node->used).insert(temp);
            temp_node->parent.insert(v[v.size() - 1]);
        }
    }

    if (!control_dependence.empty()) {
        struct node* parent = control_dependence.top();
        if (in_parallel_flag) {
            stringstream ss1(number);
            string temp1;
            getline(ss1, temp1, '@');
            getline(ss1, temp1, '@');
            int num = stoi(temp1);
            if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
                parent = thread_control_dependence[num].top();
            }
        }
        temp_node->parent.insert(parent);
    }
    code[number] = temp_node;

    return_statements[control_dependence.top()] = temp_node;

    // to set data depenedence for formal out node
    struct node* function_node = control_dependence.top();
    vector<struct node*> v = function_node->formal_out_nodes;
    for (int i = 0; i < v.size(); i++) {
        string temp = v[i]->statement;
        int j = 0;
        while (j < temp.length() && temp[j] != '*') {
            j++;
        }
        j++;
        temp = temp.substr(j);
        vector<struct node*> v1;
        if (threads_variable_name_to_nodes_defined.size() > thread_number && threads_variable_name_to_nodes_defined[thread_number].find(temp) != threads_variable_name_to_nodes_defined[thread_number].end()) {
            v1 = threads_variable_name_to_nodes_defined[thread_number][temp];
        }
        else {
            v1 = variable_name_to_nodes[temp];
        }
        v[i]->parent.insert(v1[v1.size() - 1]);
    }
}

void find_shared_variables() {
    for (auto it = variable_name_to_nodes.begin(); it != variable_name_to_nodes.end(); it++) {
        if (thread_private_variables.find(it->first) == thread_private_variables.end()) {
            shared_variables.insert(it->first);
        }
    }
}

void find_interference_edges() {
    while (threads_variable_name_to_nodes_defined.size() < threads_variable_name_to_nodes_used.size()) {
        unordered_map<string, vector<struct node*>> mp;
        threads_variable_name_to_nodes_defined.push_back(mp);
    }
    while (threads_variable_name_to_nodes_defined.size() > threads_variable_name_to_nodes_used.size()) {
        unordered_map<string, vector<struct node*>> mp;
        threads_variable_name_to_nodes_used.push_back(mp);
    }
    for (int i = 0; i < threads_variable_name_to_nodes_used.size(); i++) {          // running over all of the threads
        unordered_map<string, vector<struct node*>> mp = threads_variable_name_to_nodes_used[i];
        for (auto it : shared_variables) {        // for to run over all shared variables
            if (mp.find(it) != mp.end()) {      // to check shared variable is used in the thread or not
                vector<struct node*> temp1 = mp[it];
                for (int j = 0; j < threads_variable_name_to_nodes_used.size(); j++) {   // run over all later threads
                    if (j == i) {
                        continue;
                    }
                    unordered_map<string, vector<struct node*>> mp1 = threads_variable_name_to_nodes_defined[j];
                    if (mp1.find(it) != mp1.end()) {   // to check that shared variable is defined inside another thread
                        vector<struct node*> temp2 = mp1[it];
                        for (int k = 0; k < temp1.size(); k++) {     // run over all used nodes
                            for (int l = 0; l < temp2.size(); l++) {  // run over all defined nodes
                                temp1[k]->interference_edge.push_back(temp2[l]);  // make interference edge between used -> defined nodes
                            }
                        }
                    }
                }
            }
        }
    }
}

void process_pragma(string line, int currentIndex, string number) {
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    temp_node->in_thread = true;
    pair<int, string> token = find_token(i, line);
    i = token.first;

    while (i < line.length()) {
        token = find_token(i, line);
        i = token.first;
        string temp = token.second;
        if (temp == "parallel") {
            find_shared_variables();
        }
        if (temp == "section") {
            stringstream ss1(number);
            string temp1;
            getline(ss1, temp1, '@');

            // to add marking line for section
            marking_lines["section"].push_back(number);

            // to remove eralier section from control dependence
            struct node* parent = control_dependence.top();
            if (in_parallel_flag) {
                getline(ss1, temp1, '@');
                int num = stoi(temp1);
                if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
                    parent = thread_control_dependence[num].top();
                }
            }
            string temp = parent->statement;
            pair<int, string> p = find_token(0, temp);
            p = find_token(p.first, temp);
            p = find_token(p.first, temp);
            if (p.second == "section") {
                if (in_parallel_flag) {
                    stringstream ss1(number);
                    string temp_thread_number;
                    getline(ss1, temp_thread_number, '@');
                    getline(ss1, temp_thread_number, '@');
                    int num = stoi(temp_thread_number);
                    thread_control_dependence[num].pop();
                }
                else {
                    control_dependence.pop();
                }
            }
        }
        if (temp == "shared") {
            while (i < line.length()) {
                token = find_token(i, line);
                i = token.first;
                shared_variables.insert(token.second);
            }
        }
        else if (temp == "private") {
            while (i < line.length()) {
                token = find_token(i, line);
                i = token.first;
                shared_variables.erase(token.second);
            }
        }
    }
    if (!control_dependence.empty()) {
        struct node* parent = control_dependence.top();
        if (in_parallel_flag) {
            stringstream ss1(number);
            string temp1;
            getline(ss1, temp1, '@');
            getline(ss1, temp1, '@');
            int num = stoi(temp1);
            if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
                parent = thread_control_dependence[num].top();
            }
        }
        temp_node->parent.insert(parent);
    }
    code[number] = temp_node;
}

void resolve_thread_variables(string number) {
    for (int i = 0; i < threads_variable_name_to_nodes_defined.size(); i++) {
        for (auto it : shared_variables) {
            if (threads_variable_name_to_nodes_defined[i].find(it) != threads_variable_name_to_nodes_defined[i].end()) {
                vector<struct node*> v = threads_variable_name_to_nodes_defined[i][it];
                for (int j = 0; j < v.size(); j++) {
                    variable_name_to_nodes[it].push_back(v[j]);
                }

                // to add nodes in parallel thread nodes
                for (int k = 0; k < i; k++) {
                    if (threads_variable_name_to_nodes_defined[k].find(it) != threads_variable_name_to_nodes_defined[k].end()) {
                        vector<struct node*> v1 = threads_variable_name_to_nodes_defined[k][it];
                        v[v.size() - 1]->parallel_thread_nodes.push_back(v1[v1.size() - 1]);
                    }
                }
            }
        }
    }

    // for thread_private variables
    if (thread_private_variables.size() > 0) {
        for (auto it : thread_private_variables) {
            if (threads_variable_name_to_nodes_defined[0].find(it) != threads_variable_name_to_nodes_defined[0].end()) {
                // cout << "in thread private variable : " << number << endl;
                vector<struct node*> v1 = threads_variable_name_to_nodes_defined[0][it];
                for (int i = 0; i < v1.size(); i++) {
                    variable_name_to_nodes[it].push_back(v1[i]);
                }
            }
        }
    }

    find_interference_edges();

    threads_variable_name_to_nodes_defined.clear();
    threads_variable_name_to_nodes_used.clear();
}

void process(string line, string number) {
    int i = 0;
    while (i < line.length() && line[i] == ' ') {
        i++;
    }
    if (i < line.length() && line[i] == '/' && i + 1 < line.length() && line[i + 1] == '/') {
        return;
    }

    i = 0;
    pair<int, string> token = find_token(i, line);
    i = token.first;
    string temp = token.second;
    if (keywords.find(temp) != keywords.end()) {
        switch (keywords[temp]) {
        case 0:
            process_for(line, i, number);
            break;
        case 1:
            process_while(line, i, number);
            break;
        case 2:
            process_if(line, i, number);
            break;
        case 3:
            process_else(line, i, number);
            break;
        case 4:
            process_cin(line, i, number);
            break;
        case 5:
            process_cout(line, i, number);
            break;
        case 6:
            process_return(line, i, number);
            break;
        case 7:
            while (i < line.length()) {
                token = find_token(i, line);
                i = token.first;
                temp = token.second;
                if (temp == "threadprivate") {
                    while (i < line.length()) {
                        token = find_token(i, line);
                        i = token.first;
                        if (token.second != "") {
                            shared_variables.erase(token.second);
                            thread_private_variables.insert(token.second);
                        }
                    }
                    break;
                }
                else if (temp == "barrier") {
                    if (visited_lines.find(number) == visited_lines.end()) {
                        visited_lines.insert(number);
                        resolve_thread_variables(number);
                    }
                    break;
                }
            }
            break;
        }
    }
    else {
        string temp1 = "";
        int j = 0;
        while (j < line.length() && line[j] == ' ') {
            j++;
        }
        while (j < line.length() && line[j] != ' ') {
            temp1 += line[j++];
        }
        if (temp1 == "{") { // to mark enterd into scope
            string temp2, temp3;
            stringstream temp2_line(number);
            getline(temp2_line, temp2, '_');
            int num;
            pair<int, string> p = find_token(0, source_code[stoi(temp2) - 1]);
            if (p.second == "while" || p.second == "for") { // to check that it is loop and not if else statement
                getline(temp2_line, temp3, '_');
                string temp4 = to_string(stoi(temp2) - 1) + '_' + temp3;
                if (p.second == "while")
                    while_loop_flag = 1;
                else {
                    string temp1;
                    vector<string> tokenized_line;
                    stringstream temp1_line(code[temp4]->statement);
                    while (getline(temp1_line, temp1, ';')) {
                        tokenized_line.push_back(temp1);
                    }
                    process_definition(tokenized_line[2].substr(0, tokenized_line[2].length() - 1), 0, temp4);
                    // process for loops increamenting condition as this will execute only when we enter into loop
                    for_loop_flag = 1;
                }
                struct node* temp_node = code[temp4];
                if (!control_dependence.empty()) {
                    struct node* parent = control_dependence.top();
                    if (in_parallel_flag) {
                        stringstream ss1(number);
                        string temp1;
                        getline(ss1, temp1, '@');
                        getline(ss1, temp1, '@');
                        num = stoi(temp1);
                        if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
                            parent = thread_control_dependence[num].top();
                        }
                    }
                    temp_node->parent.insert(parent);
                }
                if (in_parallel_flag) {
                    stringstream ss1(number);
                    string temp_thread_number;
                    getline(ss1, temp_thread_number, '@');
                    getline(ss1, temp_thread_number, '@');
                    num = stoi(temp_thread_number);
                    while (thread_control_dependence.size() < num + 1) {
                        stack<struct node*> st;
                        thread_control_dependence.push_back(st);
                    }
                    thread_control_dependence[num].push(temp_node);
                }
                else {
                    control_dependence.push(temp_node);
                }
            }
            else if (p.second == "if" || p.second == "else") {
                if (for_loop_flag || while_loop_flag) {
                    string temp2, temp3;
                    stringstream temp2_line(number);
                    getline(temp2_line, temp2, '_');
                    getline(temp2_line, temp3, '_');
                    string temp = to_string(stoi(temp2) - 1) + "_" + temp3;

                    if (in_parallel_flag) {
                        stringstream ss1(number);
                        string temp_thread_number;
                        getline(ss1, temp_thread_number, '@');
                        getline(ss1, temp_thread_number, '@');
                        num = stoi(temp_thread_number);
                        while (thread_control_dependence.size() < num + 1) {
                            stack<struct node*> st;
                            thread_control_dependence.push_back(st);
                        }
                        thread_control_dependence[num].push(code[temp]);
                    }
                    else {
                        control_dependence.push(code[temp]);
                    }
                }
                else {
                    string temp2, temp3;
                    stringstream temp2_line(number);
                    getline(temp2_line, temp2, '@');
                    string temp = to_string(stoi(temp2) - 1);
                    if (getline(temp2_line, temp3, '@')) {
                        temp += "@" + temp3;
                    }
                    if (in_parallel_flag) {
                        stringstream ss1(number);
                        string temp_thread_number;
                        getline(ss1, temp_thread_number, '@');
                        getline(ss1, temp_thread_number, '@');
                        num = stoi(temp_thread_number);
                        while (thread_control_dependence.size() < num + 1) {
                            stack<struct node*> st;
                            thread_control_dependence.push_back(st);
                        }
                        thread_control_dependence[num].push(code[temp]);
                    }
                    else {
                        control_dependence.push(code[temp]);
                    }
                }
            }
            else if (p.second == "pragma") {
                in_parallel_flag++;
                string temp2, temp3;
                stringstream temp2_line(number);
                getline(temp2_line, temp2, '@');
                string temp = to_string(stoi(temp2) - 1);
                if (getline(temp2_line, temp3, '@')) {
                    temp += "@" + temp3;
                }

                process_pragma(source_code[stoi(temp2) - 1], p.first, temp);

                if (in_parallel_flag) {
                    stringstream ss1(number);
                    string temp_thread_number;
                    getline(ss1, temp_thread_number, '@');
                    getline(ss1, temp_thread_number, '@');
                    num = stoi(temp_thread_number);
                    while (thread_control_dependence.size() < num + 1) {
                        stack<struct node*> st;
                        thread_control_dependence.push_back(st);
                    }
                    thread_control_dependence[num].push(code[temp]);
                }
                else {
                    control_dependence.push(code[temp]);
                }
            }
            else {
                string temp2, temp3;
                stringstream temp2_line(number);
                getline(temp2_line, temp2, '@');
                string temp = to_string(stoi(temp2) - 1);
                if (getline(temp2_line, temp3, '@')) {
                    temp += "@" + temp3;
                }
                control_dependence.push(code[temp]);
            }
            /*if (in_parallel_flag) {
                cout << thread_control_dependence[num].top()->line_number << " : " << thread_control_dependence[num].top()->statement << " pushed\n";
            }
            else {
                cout << control_dependence.top()->line_number << " : " << source_code[stoi(number) - 1] << " pushed\n";
            }*/
            return;
        }
        if (temp1 == "}") {
            struct node* parent = control_dependence.top();
            if (in_parallel_flag) {
                stringstream ss1(number);
                string temp1;
                getline(ss1, temp1, '@');
                getline(ss1, temp1, '@');
                int num = stoi(temp1);
                if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
                    parent = thread_control_dependence[num].top();
                }
            }

            pair<int, string> p = find_token(0, parent->statement);
            if (p.second == "pragma") {
                struct node* temp_node = new node(line, number);
                struct node* parent = control_dependence.top();
                if (in_parallel_flag) {
                    stringstream ss1(number);
                    string temp1;
                    getline(ss1, temp1, '@');
                    getline(ss1, temp1, '@');
                    int num = stoi(temp1);
                    if (thread_control_dependence.size() >= num + 1 && !thread_control_dependence[num].empty()) {
                        parent = thread_control_dependence[num].top();
                    }
                }
                temp_node->parent.insert(parent);
                code[number] = temp_node;

                if (in_parallel_flag) {
                    stringstream ss1(number);
                    string temp_thread_number;
                    getline(ss1, temp_thread_number, '@');
                    getline(ss1, temp_thread_number, '@');
                    int num = stoi(temp_thread_number);
                    // cout << thread_control_dependence[num].top()->line_number << " : " << thread_control_dependence[num].top()->statement << " poped\n";
                    thread_control_dependence[num].pop();
                }
                else {
                    // cout << control_dependence.top()->line_number << " : " << control_dependence.top()->statement << " poped\n";
                    control_dependence.pop();
                }

                bool check = 1;
                for (int i = 0; i < thread_control_dependence.size(); i++) {
                    if (!thread_control_dependence[i].empty()) {
                        check = 0;
                        break;
                    }
                }

                if (check) {
                    // cout << "in check " << number << endl;
                    resolve_thread_variables(number); // clear thread variables and find interfernece edges between thread processed variables
                    thread_private_variables.clear();
                }
                in_parallel_flag--;
                return;
            }

            if (while_loop_flag)
                while_loop_flag = 0;
            if (for_loop_flag)
                for_loop_flag = 0;
            if (!in_parallel_flag && control_dependence.size() == 1) { // if all parallel portion is complete and only function is left in stack than to set data dependence for formal out nodes
                struct node* function_node = control_dependence.top();
                vector<struct node*> v = function_node->formal_out_nodes;
                for (int i = 0; i < v.size(); i++) {
                    string temp = v[i]->statement;
                    int j = 0;
                    while (j < temp.length() && temp[j] != '*') {
                        j++;
                    }
                    j++;
                    temp = temp.substr(j);
                    vector<struct node*> v1;
                    v1 = variable_name_to_nodes[temp];
                    v[i]->parent.insert(v1[v1.size() - 1]);
                }
            }

            // to pop from the control dependence stack
            if (in_parallel_flag) {
                stringstream ss1(number);
                string temp_thread_number;
                getline(ss1, temp_thread_number, '@');
                getline(ss1, temp_thread_number, '@');
                int num = stoi(temp_thread_number);
                // cout << thread_control_dependence[num].top()->line_number << " : " << thread_control_dependence[num].top()->statement << " poped\n";
                thread_control_dependence[num].pop();
            }
            else {
                // cout << control_dependence.top()->line_number << " : " << control_dependence.top()->statement << " poped\n";
                control_dependence.pop();
            }
            return;
        }

        // to find if it is function call or definition or else
        j = 0;
        while (j < line.length() && line[j] == ' ') {
            j++;
        }
        temp1 = "";
        bool return_value_expects = 0;
        while (j < line.length() && line[j] != '(') {
            if (line[j] == '=') {
                return_value_expects = 1;
            }
            if (line[j] != '(') {
                temp1 += line[j];
            }
            j++;
        }
        if (j < line.length()) {
            if (return_value_expects) {
                process_function_call(line, number, j, 1);
            }
            else if (function_map.find(temp1) != function_map.end()) {
                process_function_call(line, number, j, 0);
            }
            else if (j < line.length()) {
                int l = j;
                while (l < line.length() && line[l] != ';') {
                    l++;
                }
                if (l == line.length()) {
                    process_function_definition(line, number, j);
                }
            }
        }
        else {
            process_definition(line, 0, number);
        }
    }
}

bool cmp1(struct node* a, struct node* b) {
    return a->line_number < b->line_number;
}

bool cmp(string a, string b) {
    string temp1, temp2;
    vector<string> c_temp, d_temp;
    stringstream temp1_line(a), temp2_line(b);
    while (getline(temp1_line, temp1, '_')) {
        c_temp.push_back(temp1);
    }
    while (getline(temp2_line, temp2, '_')) {
        d_temp.push_back(temp2);
    }
    if(c_temp.size() > 1 && d_temp.size() > 1) {
        stringstream ss1(c_temp[1]), ss2(d_temp[1]);
        vector<string> temp3, temp4;
        while (getline(ss1, temp1, '@')) {
            temp3.push_back(temp1);
        }
        while (getline(ss2, temp2, '@')) {
            temp4.push_back(temp2);
        }
        if(stoi(temp3[0]) < stoi(temp4[0])) {
            return 1;
        } else if(stoi(temp3[0]) == stoi(temp4[0])) {
            return stoi(c_temp[0]) < stoi(d_temp[0]);
        } else {
            return 0;
        }
    } else {
        return stoi(c_temp[0]) < stoi(d_temp[0]);
    }
    // string temp1, temp2, temp3, temp4;
    // vector<int> a_temp, b_temp;
    // vector<string> c_temp, d_temp;
    // stringstream temp3_line(a), temp4_line(b);
    // while (getline(temp3_line, temp3, '@')) {
    //     c_temp.push_back(temp3);
    // }
    // while (getline(temp4_line, temp4, '@')) {
    //     d_temp.push_back(temp4);
    // }

    // stringstream temp1_line(c_temp[0]), temp2_line(d_temp[0]);

    // while (getline(temp1_line, temp1, '_')) {
    //     a_temp.push_back(stoi(temp1));
    // }
    // while (getline(temp2_line, temp2, '_')) {
    //     b_temp.push_back(stoi(temp2));
    // }

    // if (c_temp.size() > 1 && d_temp.size() > 1) {
    //     if (a_temp.size() > 1 && b_temp.size() > 1) {
    //         if (c_temp[1] == d_temp[1] && a_temp[1] == b_temp[1]) { // if thread_number is same and loop_count is same than sort acc. to line_number
    //             return a_temp[0] < b_temp[0];
    //         }
    //         return a_temp[1] < b_temp[1]; // else sort acc. to loop_count
    //     }
    //     return a_temp[0] < b_temp[0]; // if any one of them is not part of loop than sort acc. line_number
    // }
    // if (c_temp.size() > 1 || d_temp.size() > 1) { // if any one of them is not part of thread than sort acc. line_number
    //     return a_temp[0] < b_temp[0];
    // }

    // // if none of them are part of thread
    // if (a_temp.size() > 1 && b_temp.size() > 1) {
    //     if (a_temp[1] == b_temp[1]) { // if for both loop_count is same than sort acc. to line_number
    //         return a_temp[0] < b_temp[0];
    //     }
    //     return a_temp[1] < b_temp[1]; // else sort acc loop_counter
    // }
    // if (a_temp[0] < b_temp[0]) {
    //     return 1;
    // }
    // return 0;
}

void find_trace(string line_number) {
    ifstream fin;
    fin.open("result.txt");
    string temp;
    getline(fin, temp);
    string temp1;
    stringstream temp_line(temp);
    vector<string> lines;
    unordered_set<string> temp_lines;
    while (getline(temp_line, temp1, '#')) {
        if (temp_lines.find(temp1) == temp_lines.end()) {
            temp_lines.insert(temp1);
            lines.push_back(temp1);
        }
    }

    sort(lines.begin(), lines.end(), cmp);

    for (int i = 0; i < lines.size(); i++) {
        string temp2;
        stringstream temp2_line(lines[i]);
        getline(temp2_line, temp2, '_');
        // cout << lines[i]<<" -> "<< source_code[stoi(temp2)] << "in find trace"<<endl;
        process(source_code[stoi(temp2)], lines[i]);
        // cout << lines[i] << " -> " << source_code[stoi(temp2)] << "in find trace"<<endl;
        stringstream ss1(lines[i]), ss2(line_number);
        string num1, num2;
        getline(ss1, num1, '@');
        getline(ss2, num2, '@');
        if (cmp(num1, num2) && code.find(lines[i]) != code.end()) {
            code[lines[i]]->mark = true;
        }
    }
}

void show_output(string line_number, vector<string> variable_names) {
    struct node* temp_node = code[line_number];

    unordered_set<struct node*> ans;
    queue<struct node*> q;

    for (int i = 0; i < variable_names.size(); i++) {
        if (temp_node->defined.find(variable_names[i]) != temp_node->defined.end()) {
            q.push(temp_node);
            break;
        }
    }
    if (q.empty()) {
        for (int i = 0; i < variable_names.size(); i++) {
            if (temp_node->used.find(variable_names[i]) != temp_node->used.end()) {
                unordered_set<struct node*> parent = temp_node->parent;
                for (auto it : parent) {
                    if (it->defined.find(variable_names[i]) != it->defined.end()) {
                        q.push(it);
                    }
                }
            }
            else {
                cout << "Variable name : " << variable_names[i] << " does not exist in line number " << line_number << endl;
            }
        }
    }

    // 1st phase
    while (!q.empty()) {
        struct node* curr = q.front();

        // for same line but from different threads !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! be aware of this
        vector<struct node*> v1 = curr->parallel_thread_nodes;
        for (int i = 0; i < v1.size(); i++) {
            string s1, s2;
            stringstream ss1(v1[i]->line_number);
            stringstream ss2(curr->line_number);
            getline(ss1, s1, '@');
            getline(ss2, s2, '@');
            if (s1 == s2) {
                q.push(v1[i]);
            }
        }

        ans.insert(curr);
        q.pop();
        unordered_set<struct node*> v = curr->parent;

        //to find control and data dependent nodes
        for (auto it : v) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> " << it->line_number << " : " <<it->statement<<" show output control, data"<<endl;
            if (ans.find(it) == ans.end()) {
                q.push(it);
            }
        }

        //to find interference dependence nodes
        v1 = curr->interference_edge;
        for (int i = 0; i < v1.size(); i++) {
            /*cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<v1[i]->statement<<" show output intereference"<<endl;*/
            if (ans.find(v1[i]) == ans.end()) {
                q.push(v1[i]);
            }
        }

        // to find parameter in nodes
        if (curr->parametere_in_edge != NULL && ans.find(curr->parametere_in_edge) == ans.end()) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> " << curr->parametere_in_edge->line_number << " : " <<curr->parametere_in_edge->statement<<" show output parameter"<<endl;
            q.push(curr->parametere_in_edge);
        }

        // to find calling nodes
        if (curr->calling_edge != NULL && ans.find(curr->calling_edge) == ans.end()) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> " << curr->calling_edge->line_number << " : " <<curr->calling_edge->statement<<" show output calling"<<endl;
            q.push(curr->calling_edge);
        }

        // to find transitive dependent nodes
        v1 = curr->transitive_edge;
        for (int i = 0; i < v1.size(); i++) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> " << v1[i]->line_number << " : " <<v1[i]->statement<<" show output transitive"<<endl;
            if (ans.find(v1[i]) == ans.end()) {
                q.push(v1[i]);
            }
        }

        //affect return edges
        v1 = curr->affect_return_edge;
        for (int i = 0; i < v1.size(); i++) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> " << v1[i]->line_number << " : " <<v1[i]->statement<<" show output affect return"<<endl;
            if (ans.find(v1[i]) == ans.end()) {
                q.push(v1[i]);
            }
        }
    }

    // cout<<"1st phase complete\n";

    // 2nd phase 
    // add all visited nodes
    for (auto it : ans) {
        q.push(it);
    }

    while (!q.empty()) {
        struct node* curr = q.front();
        q.pop();

        // for same line but from different threads !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! be aware of this
        vector<struct node*> v1 = curr->parallel_thread_nodes;
        for (int i = 0; i < v1.size(); i++) {
            string s1, s2;
            stringstream ss1(v1[i]->line_number);
            stringstream ss2(curr->line_number);
            getline(ss1, s1, '@');
            getline(ss2, s2, '@');
            if (s1 == s2) {
                if (ans.find(v1[i]) == ans.end()) {
                    ans.insert(v1[i]);
                    q.push(v1[i]);
                }
            }
        }

        unordered_set<struct node*> v = curr->parent;

        //to find control and data dependent nodes
        for (auto it : v) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> " << it->line_number << " : " <<it->statement<<" show output control, data"<<endl;
            if (ans.find(it) == ans.end()) {
                ans.insert(it);
                q.push(it);
            }
        }

        //to find interference dependence nodes
        v1 = curr->interference_edge;
        for (int i = 0; i < v1.size(); i++) {
            /*cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<v1[i]->statement<<" show output intereference"<<endl;*/
            if (ans.find(v1[i]) == ans.end()) {
                q.push(v1[i]);
                ans.insert(v1[i]);
            }
        }

        // to find parameter out nodes
        if (curr->parametere_out_edge != NULL && ans.find(curr->parametere_out_edge) == ans.end()) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> " << curr->parametere_out_edge->line_number << " : " <<curr->parametere_out_edge->statement<<" show output parameter"<<endl;
            ans.insert(curr->parametere_out_edge);
            q.push(curr->parametere_out_edge);
        }

        // to find transitive dependent nodes
        v1 = curr->transitive_edge;
        for (int i = 0; i < v1.size(); i++) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> " << v1[i]->line_number << " : " <<v1[i]->statement<<" show output transitive"<<endl;
            if (ans.find(v1[i]) == ans.end()) {
                ans.insert(v1[i]);
                q.push(v1[i]);
            }
        }

        // affect return nodes
        v1 = curr->affect_return_edge;
        for (int i = 0; i < v1.size(); i++) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> " << v1[i]->line_number << " : " <<v1[i]->statement<<" show output affect return"<<endl;
            if (ans.find(v1[i]) == ans.end()) {
                ans.insert(v1[i]);
                q.push(v1[i]);
            }
        }

        // return link edge
        if (curr->return_link != NULL && ans.find(curr->return_link) == ans.end()) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<< curr->return_link->line_number<<" : "<<curr->return_link->statement<<" show output return link"<<endl;
            ans.insert(curr->return_link);
            q.push(curr->return_link);
        }
    }

    unordered_set<string> temp_line_numbers;

    for (auto it : ans) {
        temp_line_numbers.insert(it->line_number);
    }

    // for sections line
    vector<int> v1 = dependent_lines["section"];
    vector<string> v2 = marking_lines["section"];
    for (int i = 0; i < v2.size(); i++) {
        stringstream ss(v2[i]);
        string temp_s;
        getline(ss, temp_s, '@');
        int cmp = stoi(temp_s);
        if (temp_line_numbers.find(v2[i]) != temp_line_numbers.end()) {
            int last = 0;
            for (int j = 0; j < v1.size(); j++) {
                if (v1[j] > cmp) {
                    break;
                }
                last = v1[j];
            }

            struct node* temp_node;
            if (code.find(to_string(last)) != code.end()) {
                temp_node = code[to_string(last)];
            }
            else {
                temp_node = new node(to_string(last), source_code[last]);
            }

            struct node* temp1 = code[v2[i]];
            unordered_set<node*> v = temp1->parent;
            for (auto it : v) {
                pair<int, string> p = find_token(0, it->statement);
                if (p.second == "if" || p.second == "else" || p.second == "pragma") {
                    temp_node->parent.insert(it);
                    temp1->parent.erase(it);
                    temp1->parent.insert(temp_node);
                }
            }

            if (code.find(to_string(last)) == code.end()) {
                code[to_string(last)] = temp_node;
                ans.insert(temp_node);
            }
        }
    }

    // for 'for' line
    v1 = dependent_lines["for"];
    v2 = marking_lines["for"];
    for (int i = 0; i < v2.size(); i++) {
        string temp = v2[i];
        stringstream ss(temp);
        string temp_s;
        getline(ss, temp_s, '_');
        int cmp = stoi(temp_s);
        if (temp_line_numbers.find(temp) != temp_line_numbers.end()) {
            int last = 0;
            for (int j = 0; j < v1.size(); j++) {
                if (v1[j] > cmp) {
                    break;
                }
                last = v1[j];
            }
            struct node* temp_node;
            if (code.find(to_string(last)) != code.end()) {
                temp_node = code[to_string(last)];
            }
            else {
                temp_node = new node(to_string(last), source_code[last]);
            }

            struct node* temp1 = code[temp];
            unordered_set<node*> v = temp1->parent;
            for (auto it : v) {
                pair<int, string> p = find_token(0, it->statement);
                if (p.second == "if" || p.second == "else" || p.second == "pragma") {
                    temp_node->parent.insert(it);
                    temp1->parent.erase(it);
                    temp1->parent.insert(temp_node);
                }
            }

            if (code.find(to_string(last)) == code.end()) {
                code[to_string(last)] = temp_node;
                ans.insert(temp_node);
            }
        }
    }

    vector<struct node*> ans_temp;
    for (auto it : ans) {
        ans_temp.push_back(it);
    }

    sort(ans_temp.begin(), ans_temp.end(), cmp1);

    ofstream CD, T, I, AR, PI, PO, CE, RL, slice;
    CD.open("CD.txt");
    T.open("T.txt");
    I.open("I.txt");
    AR.open("AR.txt");
    PI.open("PI.txt");
    PO.open("PO.txt");
    CE.open("CE.txt");
    RL.open("RL.txt");
    slice.open("slice.txt");
    for (auto it : ans_temp)
    {
        slice << it->line_number << ":" << it->statement << "\n";
        CD << it->line_number << ":" << it->statement;
        for (auto iter : it->parent)
            CD << "|" << iter->line_number << ":" << iter->statement;
        CD << "\n";
        T << it->line_number << ":" << it->statement;
        for (auto iter : it->transitive_edge)
            T << "|" << iter->line_number << ":" << iter->statement;
        T << "\n";
        I << it->line_number << ":" << it->statement;
        for (auto iter : it->interference_edge)
            I << "|" << iter->line_number << ":" << iter->statement;
        I << "\n";
        AR << it->line_number << ":" << it->statement;
        for (auto iter : it->affect_return_edge)
            AR << "|" << iter->line_number << ":" << iter->statement;
        AR << "\n";
        PI << it->line_number << ":" << it->statement;
        if (it->parametere_in_edge)
            PI << "|" << it->parametere_in_edge->line_number << ":" << it->parametere_in_edge->statement;
        PI << "\n";
        PO << it->line_number << ":" << it->statement;
        if (it->parametere_out_edge)
            PO << "|" << it->parametere_out_edge->line_number << ":" << it->parametere_out_edge->statement;
        PO << "\n";
        CE << it->line_number << ":" << it->statement;
        if (it->calling_edge)
            CE << "|" << it->calling_edge->line_number << ":" << it->calling_edge->statement;
        CE << "\n";
        RL << it->line_number << ":" << it->statement;
        if (it->return_link)
            RL << "|" << it->return_link->line_number << ":" << it->return_link->statement;
        RL << "\n";
    }
    slice<<flush;
    CD<<flush;
    T<<flush;
    I<<flush;
    AR<<flush;
    CE<<flush;
    PI<<flush;
    PO<<flush;
    RL<<flush;
    
    slice.close();
    CD.close();
    T.close();
    I.close();
    AR.close();
    CE.close();
    PI.close();
    PO.close();
    RL.close();
}

int can_insert(string line, int number) {
    int i = 0;
    string temp = "";
    while (i < line.length() && line[i] == ' ') {
        i++;
    }

    int l = 0;
    pair<int, string> p;

    p = find_token(0, line);
    if (p.second == "pragma" && in_parallel_flag) {
        while (l < line.length()) {
            p = find_token(l, line);
            l = p.first;
            if (p.second == "section") {
                return 0;
            }
        }
        return 9;
    }
    else if (p.second == "pragma") {
        l = 0;
        // cout << "error : " << line << " : " << number << endl;
        while (l < line.length()) {
            p = find_token(l, line);
            l = p.first;
            if (p.second == "threadprivate") {
                return 9;
            }
        }
    }

    if (i < line.length() && line[i] == '{') {
        if (has_sections) { // increment only if sections construct encountered
            count_for_sections++;
        }
        if (has_atomic || has_critical) { // increment only if atomic or critical construct encountered
            brackets++;
        }
        string temp = source_code[number - 1];

        // to skip processing of sections,barrier,atomic,critical,flush construct related '{'
        l = 0;
        while (l < temp.length()) {
            p = find_token(l, temp);
            l = p.first;
            if (p.second == "sections" || p.second == "atomic") {
                if (p.second == "sections") {
                    dependent_lines["section"].push_back(number - 1);
                }
                return 0;
            }
        }

        pair<int, string> p = find_token(0, temp);
        if (p.second == "pragma") {
            control_dependence2.push(temp);
            in_parallel_flag++;
            return 1;
        }
        int j = 0;
        while (j < temp.length() && temp[j] == ' ') {
            j++;
        }
        string temp1 = "";
        while (j < temp.length() && temp[j] <= 122 && temp[j] >= 97) {
            temp1 += temp[j++];
        }

        if (temp1 == "for") {
            for_loop_flag = 1;
        }
        else if (temp1 == "while") {
            while_loop_flag = 1;
        }
        else if (temp1 == "if" || temp1 == "else") {
            condition_flag = 1;
        }

        if (temp1 == "for" || temp1 == "while" || temp1 == "if" || temp1 == "else") {
            control_dependence2.push(temp1);
        }
        else {
            temp1 = "";
            while (j < temp.length() && temp[j] == ' ') {
                j++;
            }
            while (j < temp.length() && temp[j] <= 122 && temp[j] >= 97) {
                temp1 += temp[j++];
            }
            function_starting = 1;
            if (temp1 == "main") {
                control_dependence2.push("main_function");
            }
            else {
                control_dependence2.push("other_function");
            }
        }
        return 1;
    }

    p = find_token(0, line);
    temp = p.second;

    if (temp == "include" || temp == "using") {
        pair<int, string> p1 = find_token(p.first, line);
        pair<int, string> p2 = find_token(p1.first, line);
        if (p1.second == "namespace" && p2.second == "std" && !functions_added) {
            functions_added = 1;
            return 8;
        }
        return 0;
    }

    if (control_dependence2.empty()) {
        return 0;
    }

    if (i < line.length() && line[i] == '}') {
        if (has_sections) {
            count_for_sections--;
        }
        if (has_atomic || has_critical) {
            brackets--;
        }
        if (has_sections && count_for_sections == 0) {
            has_sections = 0;
            return 0;
        }
        if ((has_atomic || has_critical) && brackets == 0) {
            if (has_atomic) {
                has_atomic = 0;
            }
            else {
                has_critical = 0;
            }
        }
        return 4;
    }

    if (temp == "if") {
        return 6;
    }

    if (temp == "return") {
        return 10;
    }

    if (temp == "else") {
        return 7;
    }

    if (temp == "while" || temp == "for") {
        return 5;
    }

    while (i < line.length() && line[i] != '(') {
        i++;
    }

    if (i < line.length()) {
        if (line[line.length() - 1] == ';') {
            return 2;
        }
        return 0;
    }
    if (control_dependence2.empty()) {
        return 0;
    }
    // cout << "error : " << line << " : " << number << endl;
    return 2;
}

void add_lines_from_header_file(string temp) {
    ifstream fin;
    fin.open(temp);
    string line;
    while (!fin.eof()) {
        total_number_of_lines_of_header_files++;
        getline(fin, line);
        pair<int, string> p = find_token(0, line);
        int i = 1;
        while (i < line.length() && line[i] == ' ') {
            i++;
        }
        string temp = "";
        while (i < line.length() && line[i] >= 97 && line[i] <= 122) {
            temp += line[i++];
        }

        if (temp == "include") {
            i++;
            temp = "";
            while (i < line.length() && ((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90) || line[i] == '.')) {
                temp += line[i++];
            }
            ifstream fin2;
            fin2.open(temp);
            if (fin2.is_open()) {
                fin2.close();
                add_lines_from_header_file(temp);
            }
            else {
                temp_source << line + "\n";
            }
        }
        else {
            temp_source << line + "\n";
        }
    }
    total_number_of_lines_of_header_files--;
    fin.close();
}

int main() {
    keywords_init();
    source_code.push_back("");
    string line;
    ifstream fin;
    ifstream infile;
    infile.open("fileloc");
    string filelocation;
    getline(infile,filelocation);
    fin.open(filelocation);

    temp_source.open("temp_source.cpp");
    // to add lines of source code to output file which is helpful in case of importing functions from header files
    while (!fin.eof()) {
        getline(fin, line);
        int i = 1;
        while (i < line.length() && line[i] == ' ') {
            i++;
        }
        string temp = "";
        while (i < line.length() && line[i] >= 97 && line[i] <= 122) {
            temp += line[i++];
        }

        if (temp == "include") {
            i++;
            temp = "";
            while (i < line.length() && ((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90) || line[i] == '.')) {
                temp += line[i++];
            }
            ifstream fin2;
            fin2.open(temp);
            if (fin2.is_open()) {
                fin2.close();
                add_lines_from_header_file(temp);
            }
            else {
                temp_source << line + "\n";
            }
        }
        else {
            temp_source << line + "\n";
        }
    }

    fin.close();
    temp_source.close();
    fin.open("temp_source.cpp");

    ofstream output;
    output.open("output.cpp");
    output << "#include<sstream>\n#include<fstream>\n#include<iostream>\n#include<stdio.h>\n#include <string>\n#include<conio.h>\n#include<iostream>\n#include<omp.h>\n#include<vector>\n";
    /*string slicing_line_number;
    int num;
    cout << "Enter line number where you wnat to find slice(format in case of loop is [line number]_[iteration number starting at 0]) : ";
    cin >> slicing_line_number;
    cout << "Enter number of variables : ";
    cin >> num;
    vector<string> variable_names(num);
    for (int i = 0; i < num; i++) {
        cout << "Enter variable name : ";
        cin >> variable_names[i];
    }
    cout << "Source file parsing is in progress...\n";*/
    int line_count = 0;
    bool loop_closed = 0;

    bool file_closed = 0;

    while (!fin.eof()) {
        getline(fin, line);
        source_code.push_back(line);
        line_count++;
        int flag = can_insert(line, line_count);
        pair<int, string> p;
        if (!control_dependence2.empty()) {
            p = find_token(0, control_dependence2.top());
        }
        if (flag == 2) {   // general case
            string temp_line = "";
            if (while_loop_flag || for_loop_flag) {
                temp_line = "\"" + to_string(line_count) + "_\"" + "<<to_string(loop_counter)";
                if (in_parallel_flag) {
                    temp_line = temp_line + "<<\"@\"" + "<<to_string(omp_get_thread_num())";
                    if (!has_atomic && !has_critical) {
                        output << "#pragma omp critical\n";
                        output << "{\n";
                    }
                }
                temp_line += "<<\"#\"";
            }
            else {
                temp_line = "\"" + to_string(line_count) + "\"";
                if (in_parallel_flag) {
                    temp_line = temp_line + "<<\"@\"" + "<<to_string(omp_get_thread_num())";
                    if (!has_atomic && !has_critical) {
                        output << "#pragma omp critical\n";
                        output << "{\n";
                    }
                }
                temp_line += "<<\"#\"";
            }
            output << "result<<" + temp_line + ";\n";
            if (in_parallel_flag) {
                if (!has_atomic && !has_critical) {
                    output << "}\n";
                }
            }
            output << line + "\n";
        }
        else if (flag == 1) { // "{" case
            output << line + "\n";

            if (p.second == "main_function") {
                string temp_line;
                temp_line = for_main_file_opening_line + "\"" + to_string(line_count - 1) + "#\";\nresult<<\"" + to_string(line_count) + "\"";
                if (in_parallel_flag) {
                    temp_line = temp_line + "<<\"@\"" + "<<to_string(omp_get_thread_num())";
                    if (!has_atomic && !has_critical) {
                        output << "#pragma omp critical\n";
                        output << "{\n";
                    }
                }
                temp_line += "<<\"#\";\n";
                output << temp_line;
                if (in_parallel_flag) {
                    if (!has_atomic && !has_critical) {
                        output << "}\n";
                    }
                }
                output << "string slicing_line_number;\n";
                output << "int size;\n";
                // output << "cout << \"Enter line number where you wnat to find slice(format in case of loop is[line number]_[iteration number starting at 0]) : \";\n";
                output << "cin >> slicing_line_number;\n";
                // output << "cout << \"Enter number of variables : \";\n";
                output << "cin >> size;\n";
                output << "vector<string> variable_names(size);\n";
                output << "for (int i = 0; i < size; i++) {\n";
                // output << "cout << \"Enter variable name : \";\n";
                output << "cin >> variable_names[i];\n";
                output << "}\n";
                // output << "cout << \"Source file parsing is in progress...\"<<endl;\n";
                output << "ofstream input;\n";
                output << "input.open(\"input.txt\");\n";
                output << "input << slicing_line_number << \"#\";\n";
                output << "input << size << \"#\";\n";
                output << "for (int i = 0; i < size; i++) {\n";
                output << "input << variable_names[i] << \"#\";\n";
                output << "}\n";
                output << "input.close();\n";
            }
            else if (p.second == "other_function") {
                string temp_line;
                temp_line = file_opening_line + "\"" + to_string(line_count - 1) + "#\";\nresult<<\"" + to_string(line_count) + "\"";
                if (in_parallel_flag) {
                    temp_line = temp_line + "<<\"@\"" + "<<to_string(omp_get_thread_num())";
                    if (!has_atomic && !has_critical) {
                        output << "#pragma omp critical\n";
                        output << "{\n";
                    }
                }
                temp_line += "<<\"#\";\n";
                output << temp_line;
                if (in_parallel_flag) {
                    if (!has_atomic && !has_critical) {
                        output << "}\n";
                    }
                }
            }
            else if (p.second == "pragma") {
                if (!has_atomic && !has_critical) {
                    output << "#pragma omp critical\n";
                    output << "{\n";
                }

                // in case of sections to write critical construct for atomic write in file which is not allowed to write saparately in sections
                pair<int, string> p = find_token(0, source_code[line_count - 1]);
                p = find_token(p.first, source_code[line_count - 1]);
                p = find_token(p.first, source_code[line_count - 1]);
                if (p.second == "section" || p.second == "critical" || p.second == "atomic") {
                    output << "result<<\"" + to_string(line_count - 1) + "\" << \"@\"<<omp_get_thread_num()<<\"" + "#" + "\";\n";
                }

                output << "result<<\"" + to_string(line_count) + "\" << \"@\"<<omp_get_thread_num()<<\"" + "#" + "\";\n";
                if (!has_atomic && !has_critical) {
                    output << "}\n";
                }
            }
            else if (p.second == "for" || p.second == "while") {
                string temp_line = "\"" + to_string(line_count - 1) + "_\"<<to_string(loop_counter)";
                string temp_line1 = "\"" + to_string(line_count) + "_\"<<to_string(loop_counter)";
                if (in_parallel_flag) {
                    temp_line = temp_line + "<<\"@\"" + "<<to_string(omp_get_thread_num())";
                    temp_line1 = temp_line1 + "<<\"@\"" + "<<to_string(omp_get_thread_num())";
                    if (!has_atomic && !has_critical) {
                        output << "#pragma omp critical\n";
                        output << "{\n";
                    }
                }
                temp_line += "<<\"#\"";
                temp_line1 += "<<\"#\"";
                output << "result<<" + temp_line + ";\n";
                output << "result<<" + temp_line1 + ";\n";
                if (in_parallel_flag) {
                    if (!has_atomic && !has_critical) {
                        output << "}\n";
                    }
                }

                if (p.second == "for") {
                    for_loop_flag = 1;
                }
                else {
                    while_loop_flag = 1;
                }
                loop_closed = 0;
            }
            else {
                string temp_line;
                if (while_loop_flag || for_loop_flag) {
                    loop_closed = 0;
                    temp_line = "\"" + to_string(line_count) + "_\"" + "<<to_string(loop_counter)";
                    if (in_parallel_flag) {
                        temp_line = temp_line + "<<\"@\"" + "<<to_string(omp_get_thread_num())";
                        if (!has_atomic && !has_critical) {
                            output << "#pragma omp critical\n";
                            output << "{\n";
                        }
                    }
                    temp_line += "<<\"#\"";
                }
                else {
                    temp_line = "\"" + to_string(line_count) + "\"";
                    if (in_parallel_flag) {
                        temp_line = temp_line + "<<\"@\"" + "<<to_string(omp_get_thread_num())";
                        if (!has_atomic && !has_critical) {
                            output << "#pragma omp critical\n";
                            output << "{\n";
                        }
                    }
                    temp_line += "<<\"#\"";
                }
                output << "result<<" + temp_line + ";\n";
                if (in_parallel_flag) {
                    if (!has_atomic && !has_critical) {
                        output << "}\n";
                    }
                }
            }
        }
        else if (flag == 4) { // scope closing case
            if (p.second == "if" || p.second == "else") {
                string temp_line;
                if (while_loop_flag || for_loop_flag) {
                    temp_line = "\"" + to_string(line_count) + "_\"" + "<<to_string(loop_counter)";
                    if (in_parallel_flag) {
                        temp_line = temp_line + "<<\"@\"" + "<<to_string(omp_get_thread_num())";
                        if (!has_atomic && !has_critical) {
                            output << "#pragma omp critical\n";
                            output << "{\n";
                        }
                    }
                    temp_line += "<<\"#\"";
                }
                else {
                    temp_line = "\"" + to_string(line_count) + "\"";
                    if (in_parallel_flag) {
                        temp_line = temp_line + "<<\"@\"" + "<<to_string(omp_get_thread_num())";
                        if (!has_atomic && !has_critical) {
                            output << "#pragma omp critical\n";
                            output << "{\n";
                        }
                    }
                    temp_line += "<<\"#\"";
                }
                output << "result<<" + temp_line + ";\n";
                if (in_parallel_flag) {
                    if (!has_atomic && !has_critical) {
                        output << "}\n";
                    }
                }
                condition_flag = 0;
            }
            else if (p.second == "for" || p.second == "while") {
                string temp_line = "\"" + to_string(line_count) + "_\"<<to_string(loop_counter)";
                if (in_parallel_flag) {
                    temp_line = temp_line + "<<\"@\"" + "<<to_string(omp_get_thread_num())";
                    if (!has_atomic && !has_critical) {
                        output << "#pragma omp critical\n";
                        output << "{\n";
                    }
                }
                temp_line += "<<\"#\"";
                output << "result<<" + temp_line + ";\n";
                if (in_parallel_flag) {
                    if (!has_atomic && !has_critical) {
                        output << "}\n";
                    }
                }
                output << "loop_counter = loop_counter + 1;\n";
                if (p.second == "for") {
                    for_loop_flag = 0;
                }
                else {
                    while_loop_flag = 0;
                }
                loop_closed = 1;
            }
            else if (p.second == "pragma") {
                int j = p.first;
                while (j < control_dependence2.top().length()) {
                    p = find_token(j, control_dependence2.top());
                    j = p.first;
                    if (p.second == "sections" || p.second == "critical" || p.second == "atomic") {
                        break;
                    }
                }
                if (p.second == "sections") {
                    output << line + "\n";
                    in_parallel_flag--;
                    control_dependence2.pop();
                    continue;
                }
                else if (p.second == "critical" || p.second == "atomic") {
                    output << "result<<\"" + to_string(line_count) + "\"<<\"@\"" + "<<to_string(omp_get_thread_num())" + "<<\"#\";\n";
                    output << "}\n";
                    in_parallel_flag--;
                    control_dependence2.pop();
                    continue;
                }
                else {
                    output << "#pragma omp critical\n";
                    output << "{\n";
                    output << "result<<\"" + to_string(line_count) + "\"<<\"@\"" + "<<to_string(omp_get_thread_num())" + "<<\"#\";\n";

                    // in case of sections to write critical construct for atomic write in file which is not allowed to write saparately in sections
                    string temp = control_dependence2.top();
                    control_dependence2.pop();
                    int j = 0;
                    string next_line = control_dependence2.top();
                    while (j < next_line.length()) {
                        pair<int, string> p = find_token(j, next_line);
                        j = p.first;
                        if (p.second == "sections") {
                            output << "result<<\"" + to_string(line_count + 1) + "\"<<\"@\"" + "<<to_string(omp_get_thread_num())" + "<<\"#\";\n";
                            break;
                        }
                    }
                    control_dependence2.push(temp);

                    output << "}\n";
                    in_parallel_flag--;
                }
            }
            else {
                output << "result<<\"" + to_string(line_count) + "#\";\n";
                output << "result.close();\n";
                function_starting = 0;
            }
            output << line + "\n";
            control_dependence2.pop();
        }
        else if (flag == 5) { // loop case
            p = find_token(0, source_code[line_count - 1]);
            if (p.second != "pragma") {
                output << "int loop_counter = 0;\n";
            }
            output << line + "\n";
        }
        else if (flag == 6) { // if case
            int j = 0;
            while (j < line.length() && line[j] != '(') {
                j++;
            }
            j++;
            string temp1 = line.substr(0, j);
            string temp2 = line.substr(j);

            //to write into result file
            if (has_critical || has_atomic) {
                output << temp1 + "result<<\"" + to_string(line_count) + "\"";
                if (for_loop_flag || while_loop_flag) {
                    output << "<<\"_\"<<to_string(loop_counter)";
                }
                if (in_parallel_flag) {
                    output << "<<\"@\"<<to_string(omp_get_thread_num())";
                }
                output << "<<\"#\"" << " && " << temp2;
            }
            else {
                output << temp1 + "add_function(\"" + to_string(line_count) + "\", \"" + to_string(for_loop_flag || while_loop_flag) + "\", ";
                if (for_loop_flag || while_loop_flag) {
                    output << "loop_counter, ";
                }
                else {
                    output << "-1, ";
                }
                if (in_parallel_flag) {
                    output << "omp_get_thread_num(), \"" << to_string(in_parallel_flag) << "\") && " << temp2;
                }
                else {
                    output << "-1, \"" << to_string(in_parallel_flag) << "\") && " << temp2;
                }
            }
        }
        else if (flag == 7) { // else case
            int j = 0;
            while (j < line.length() && line[j] != '(') {
                j++;
            }
            string temp3;
            if (j == line.length()) { // if only else statement than add if part which writes into file
                if (has_critical || has_atomic) {
                    output << line + " if(" + "result<<\"" + to_string(line_count) + "\"";
                    if (for_loop_flag || while_loop_flag) {
                        output << "<<\"_\"<<to_string(loop_counter)";
                    }
                    if (in_parallel_flag) {
                        output << "<<\"@\"<<to_string(omp_get_thread_num())";
                    }
                    output << "<<\"#\")";
                }
                else {
                    output << line + " if(add_function(\"" + to_string(line_count) + "\", \"" + to_string(for_loop_flag || while_loop_flag) + "\", ";
                    if (for_loop_flag || while_loop_flag) {
                        output << "loop_counter, ";
                    }
                    else {
                        output << "-1, ";
                    }
                    if (in_parallel_flag) {
                        output << "omp_get_thread_num(), \"" << to_string(in_parallel_flag) << "\"))";
                    }
                    else {
                        output << "-1, \"" << to_string(in_parallel_flag) << "\"))";
                    }
                }
            }
            else { // for in else portion if statement exist in that case to write into result file
                j++;
                string temp1 = line.substr(0, j);
                string temp2 = line.substr(j);
                if (has_critical || has_atomic) {
                    output << temp1 + "result<<\"" + to_string(line_count) + "\"";
                    if (for_loop_flag || while_loop_flag) {
                        output << "<<\"_\"<<to_string(loop_counter)";
                    }
                    if (in_parallel_flag) {
                        output << "<<\"@\"<<to_string(omp_get_thread_num())";
                    }
                    output << "<<\"#\"" << " && " << temp2;
                }
                else {
                    output << temp1 + "add_function(\"" + to_string(line_count) + "\", \"" + to_string(for_loop_flag || while_loop_flag) + "\", ";
                    if (for_loop_flag || while_loop_flag) {
                        output << "loop_counter, ";
                    }
                    else {
                        output << "-1, ";
                    }
                    if (in_parallel_flag) {
                        output << "omp_get_thread_num(), \"" << to_string(in_parallel_flag) << "\") && " << temp2;
                    }
                    else {
                        output << "-1, \"" << to_string(in_parallel_flag) << "\") && " << temp2;
                    }
                }
            }
        }
        else if (flag == 8) { // added function for to call atomic function for if-else and loop case
            output << line + "\n";
            output << "using namespace std;\n";
            output << "bool add_function(string number, string in_loop, int loop_count, int num, string flag)\n";
            output << "{\n";
            output << "#pragma omp critical\n";
            output << "{\n";
            output << "ofstream result;\n";
            output << "result.open(\"result.txt\", ios::app);\n";
            output << "result << number;\n";
            output << "if (in_loop == \"1\") {\n";
            output << "result << \"_\" << to_string(loop_count);\n";
            output << "}\n";
            output << "if (flag != \"0\") {\n";
            output << "result << \"@\" << to_string(num);\n";
            output << "}\n";
            output << "result << \"#\";\n";
            output << "result.close();\n";
            output << "}\n";
            output << "return 1;\n";
            output << "}\n";
            output << "bool add_function_for_loop(string number, int loop_count, int num, string flag)\n";
            output << "{\n";
            output << "#pragma omp critical\n";
            output << "{\n";
            output << "ofstream result;\n";
            output << "result.open(\"result.txt\", ios::app);\n";
            output << "result << number << \"_\" << to_string(loop_count);\n";
            output << "if (flag != \"0\") {\n";
            output << "result << \"@\" << to_string(num);\n";
            output << "}\n";
            output << "result << \"#\";\n";
            output << "result.close();\n";
            output << "}\n";
            output << "return 1;\n";
            output << "}\n";
        }
        else if (flag == 9) { // to write in case of nested pragma constructs
            int j = 0;
            while (j < line.length()) {
                p = find_token(j, line);
                j = p.first;
                if (p.second == "sections" || p.second == "for" || p.second == "atomic" || p.second == "critical" || p.second == "barrier"
                    || p.second == "flush" || p.second == "threadprivate") {
                    break;
                }
            }
            if (p.second == "threadprivate") {
                output << "result<<\"" + to_string(line_count) + "\"" + "<<\"#\";\n";
                output << line + "\n";
                continue;
            }
            if (p.second == "sections") {
                has_sections = 1;
            }
            if (p.second == "atomic") {
                has_atomic = 1;
            }
            if (p.second == "critical") {
                has_critical = 1;
            }
            if (p.second == "sections" || p.second == "for" || p.second == "barrier" || p.second == "atomic" || p.second == "critical" || p.second == "flush") {
                if (p.second == "for") {
                    output << "int loop_counter = 0;\n";
                    dependent_lines["for"].push_back(line_count);
                    /*struct node* temp_node = new node(to_string(line_count), line);
                    code[to_string(line_count)] = temp_node;*/
                }
                else if (p.second == "barrier") {
                    output << "result<<\"" + to_string(line_count) + "\" <<\"" + "#" + "\";\n";
                }
                output << line + "\n";
                continue;
            }

            output << "#pragma omp critical\n";
            output << "{\n";
            output << "result<<\"" + to_string(line_count) + "\" << \"@\"<<omp_get_thread_num()<<\"" + "#" + "\";\n";
            output << "}\n";
            output << line + "\n";
        }
        else if (flag == 10) {
            output << "result<<\"" << to_string(line_count) << "\"" << "\"#\";\n";
            output << line + "\n";
            control_dependence2.pop();
        }
        else { // #, main, using case
            output << line + "\n";
        }
    }
    fin.close();
    output.close();

    string s = "g++ -o output.exe -fopenmp output.cpp";
    const char* command = s.c_str();
    system(command);
    system("output.exe");

    ifstream input;
    input.open("input.txt");
    string temp, temp1;
    getline(input, temp);
    stringstream temp_line(temp);
    getline(temp_line, temp1, '#');
    string slicing_line_number = temp1;
    getline(temp_line, temp1, '#');
    int size = stoi(temp1);
    vector<string> variable_names(size);
    for (int i = 0; i < size; i++) {
        getline(temp_line, variable_names[i], '#');
    }
    input.close();
    string temp_s2 = slicing_line_number;
    stringstream temp_ss(temp_s2);
    string temp_s;
    getline(temp_ss, temp_s, '_');
    slicing_line_number = to_string(stoi(temp_s) + total_number_of_lines_of_header_files);
    if (getline(temp_ss, temp_s, '_')) {
        slicing_line_number += "_" + temp_s;
    }
    find_trace(slicing_line_number);

    remove("./input.txt");
    remove("./output.cpp");
    remove("./output.exe");
    remove("./result.txt");
    remove("./temp_source.txt");

    if (code.find(slicing_line_number) == code.end() &&
        code.find(slicing_line_number + "_0") == code.end() &&
        code.find(slicing_line_number + "@0") == code.end() &&
        code.find(slicing_line_number + "@1") == code.end() &&
        code.find(slicing_line_number + "@2") == code.end() &&
        code.find(slicing_line_number + "@3") == code.end()) {
        cout << "You entered line number does not contains \n";
        return 0;
    }

    if (code.find(slicing_line_number) != code.end()) {
        show_output(slicing_line_number, variable_names);
    }
    else if (code.find(slicing_line_number + "@0") != code.end()) {
        show_output(slicing_line_number + "@0", variable_names);
    }
    else if (code.find(slicing_line_number + "@1") != code.end()) {
        show_output(slicing_line_number + "@1", variable_names);
    }
    else if (code.find(slicing_line_number + "@2") != code.end()) {
        show_output(slicing_line_number + "@2", variable_names);
    }
    else if (code.find(slicing_line_number + "@3") != code.end()) {
        show_output(slicing_line_number + "@3", variable_names);
    }
    else if (code.find(slicing_line_number + "_0") != code.end()) {
        show_output(slicing_line_number + "_0", variable_names);
    }
}