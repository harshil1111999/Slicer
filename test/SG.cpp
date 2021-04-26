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
unordered_map<string, vector<struct node*>> variable_name_to_nodes, temp_variable_name_to_nodes_defined, temp_variable_name_to_nodes_used; // for to find out all definition node of variable
unordered_map<string, struct node*> function_map;
unordered_map<struct node*, struct node*> return_statements;
unordered_set<string> datatypes; // for inbuilt datatypes
vector<unordered_map<string, vector<struct node*>>> threads_variable_name_to_nodes_defined, threads_variable_name_to_nodes_used; // different scope for different threads
unordered_set<string> shared_variables;
unordered_set<string> thread_private_variables; // to keep track of thread_private variables
stack<struct node*> control_dependence;
stack<string> control_dependence2; // for to create trace keep track of scope
string file_opening_line = "ofstream result;\nresult.open(\"result.txt\", ios::trunc);\nresult<<";
string for_main_file_opening_line = "ofstream result;\nresult.open(\"result.txt\", ios::app);\nresult<<";
vector<string> source_code; // to find out line by line_number directly
vector<int> conditional_statements; // to keep track of hierarchy of if else statements
bool while_loop_flag = 0, for_loop_flag = 0, function_starting = 0, condition_flag = 0;
bool replication_in_progress = 0; // to mark replication is start or not
unordered_map<string, vector<int>> marking_lines; // for dependent lines, mark lines which flags dependent lines to include
unordered_map<string, vector<int>> dependent_lines; // lines like sections and for which haven't processed but should include if marked lines are added
bool in_thread = 0;
int parallel_bracket_count = 0;
int thread_count = 0;
bool thread_count_increment = 0;
bool has_section = 0;
bool has_single_or_master = 0;
ofstream output;
int total_number_of_lines_of_header_files = 0;
pair<string,string> cloning_ends = { "0", "0"};

void process(string line, string number);

struct node {
    unordered_set<struct node*> parent; // contains control and data dependences
    vector<struct node*> parallel_thread_nodes; // after completing parallel region it contains refernce to nodes with same name and last definition from another thread
    struct node* parametere_in_edge, * parametere_out_edge, * calling_edge; // parameter IN/OUT edge
    struct node* return_link;
    vector<struct node*> transitive_edge, affect_return_edge, interference_edge;
    bool mark; // for dynamic dependence graph
    bool flag; // for transitive edge and affect return edge
    // bool in_thread; // for to create different variable scope for different threads
    bool replicate; // for to check it is only parallel, for construct
    string line_number;
    string statement;
    unordered_set<string> used, defined;
    vector<struct node*> formal_in_nodes, formal_out_nodes, actual_in_nodes, actual_out_nodes;

    node(string number, string line) {
        parametere_out_edge = NULL;
        parametere_in_edge = NULL;
        calling_edge = NULL;
        return_link = NULL;

        flag = false;
        mark = false;
        // in_thread = false;
        replicate = false;
        line_number = number;
        statement = line;
    }

};

void keywords_init() {
    keywords["for"] = 0;
    keywords["while"] = 1;
    keywords["if"] = 2;
    keywords["else"] = 3;
    keywords["cin"] = 4;
    keywords["cout"] = 5;
    keywords["return"] = 6;
    keywords["pragma"] = 7;

    datatypes.insert("int");
    datatypes.insert("float");
    datatypes.insert("double");
    datatypes.insert("string");
    datatypes.insert("void");
    datatypes.insert("static");
}

//parse words from the line
pair<int,string> find_token(int i, string line) {
    string temp = "";
    bool is_string = 0;
    while(i < line.length() && !(line[i] >= 97 && line[i] <= 122) && !(line[i] >= 65 && line[i] <= 90)
        && !(line[i] >= 48 && line[i] <= 57) && line[i] != '_' && line[i] != '"' && line[i] != '\'') {
            i++;
    }

    if(i < line.length() && line[i] == '"') {
        i++;
        while(i < line.length() && line[i] != '"') {
            i++;
        }
        i++;
        return find_token(i, line);
    }

    while(i < line.length() && ((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90) 
        || (line[i] >= 48 && line[i] <= 57) || line[i] == '_' || line[i] == '"' || line[i] == '\'')) {
            temp += line[i++];
    }
    return {i, temp};
}

// find defined and used variables and create node for the statement and push node into code map for defined variable
void process_definition(string line, int currentIndex, string number) {
    int i = currentIndex;
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
        temp_node = new node(number, line);
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
        if (in_thread && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {
            temp_variable_name_to_nodes_used[temp].push_back(temp_node);
        }

        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && temp_variable_name_to_nodes_defined.find(temp) == temp_variable_name_to_nodes_defined.end())  {
            cout << "The variable name " << temp << " does not exist in definition portion\n";
        }
        else {
            (temp_node->used).insert(temp);
            vector<struct node*> v;
            if (temp_variable_name_to_nodes_defined.find(temp) != temp_variable_name_to_nodes_defined.end()) {
                v = temp_variable_name_to_nodes_defined[temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            temp_node->parent.insert(v[v.size() - 1]);
        }
    }

    if (!control_dependence.empty()) {
        temp_node->parent.insert(control_dependence.top());
    }
    if (in_thread) {
        temp_variable_name_to_nodes_defined[variable_name].push_back(temp_node);
    }
    else {
        variable_name_to_nodes[variable_name].push_back(temp_node);
    }
    code[number] = temp_node;
}

void process_for(string line, int currentIndex, string number) {
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
    while (getline(temp2_line, temp1, '_')) {
        tokenized_number.push_back(temp1);
    }

    // add line into marking lines
    string eralier_line = source_code[stoi(tokenized_number[0]) - 1];
    pair<int, string> eralier_p = find_token(0, eralier_line);
    eralier_p = find_token(eralier_p.first, eralier_line);
    eralier_p = find_token(eralier_p.first, eralier_line);
    if (eralier_p.second == "for") {
        marking_lines["for"].push_back(stoi(tokenized_number[0]));
        dependent_lines["for"].push_back(stoi(tokenized_number[0]) - 1);
    }

    // if (tokenized_number[1] == "0") {
        process_definition(tokenized_line[0], 0, number);
    // }

    i = 0;
    while (i < tokenized_line[1].length()) {
        pair<int, string> token = find_token(i, tokenized_line[1]);

        i = token.first;
        string temp = token.second;
        if (in_thread && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {
            temp_variable_name_to_nodes_used[temp].push_back(temp_node);
        }
        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && temp_variable_name_to_nodes_defined.find(temp) == temp_variable_name_to_nodes_defined.end()) {
            cout << "The variable name " << temp << " does not exist in if portion\n";
        }
        else {
            (temp_node->used).insert(temp);
            vector<struct node*> v;
            if (temp_variable_name_to_nodes_defined.find(temp) != temp_variable_name_to_nodes_defined.end()) {
                v = temp_variable_name_to_nodes_defined[temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            temp_node->parent.insert(v[v.size() - 1]);
        }
    }
}

void process_while(string line, int currentIndex, string number) {
    // loop_flag = 1;
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;
        if (in_thread && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {
            temp_variable_name_to_nodes_used[temp].push_back(temp_node);
        }
        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && temp_variable_name_to_nodes_defined.find(temp) == temp_variable_name_to_nodes_defined.end()) {
            cout << "The variable name " << temp << " does not exist in if portion\n";
        }
        else {
            (temp_node->used).insert(temp);
            vector<struct node*> v;
            if (temp_variable_name_to_nodes_defined.find(temp) != temp_variable_name_to_nodes_defined.end()) {
                v = temp_variable_name_to_nodes_defined[temp];
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
    int i = currentIndex;
    struct node* temp_node;
    if (code.find(number) != code.end()) {
        temp_node = code[number];
    } else {
        temp_node = new node(number, line);
    }
    temp_node->parent.insert(control_dependence.top());
    // control_dependence.push(temp_node);
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;
        if (in_thread && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {
            temp_variable_name_to_nodes_used[temp].push_back(temp_node);
        }
        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && temp_variable_name_to_nodes_defined.find(temp) == temp_variable_name_to_nodes_defined.end()) {
            cout << "The variable name " << temp << " does not exist in if portion\n";
        }
        else {
            (temp_node->used).insert(temp);
            vector<struct node*> v;
            if (temp_variable_name_to_nodes_defined.find(temp) != temp_variable_name_to_nodes_defined.end()) {
                v = temp_variable_name_to_nodes_defined[temp];
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
        temp_node->parent.insert(control_dependence.top());
        // control_dependence.pop();
    }
    // control_dependence.push(temp_node);
    code[number] = temp_node;
}

void process_cin(string line, int currentIndex, string number) {
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
        if (in_thread) {
            temp_variable_name_to_nodes_defined[temp].push_back(temp_node);
        }
        else {
            variable_name_to_nodes[temp].push_back(temp_node);
        }
    }
    if (!control_dependence.empty()) {
        temp_node->parent.insert(control_dependence.top());
    }
    code[number] = temp_node;
}

//not cosidered case of "variable_name"
void process_cout(string line, int currentIndex, string number) {
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;
        if (in_thread && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {
            temp_variable_name_to_nodes_used[temp].push_back(temp_node);
        }
        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'' || temp == "endl") {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && temp_variable_name_to_nodes_defined.find(temp) == temp_variable_name_to_nodes_defined.end()) {
            cout <<number<< " : The variable name " << temp << " does not exist in cout portion\n";
        }
        else {
            vector<struct node*> v;
            if (temp_variable_name_to_nodes_defined.find(temp) != temp_variable_name_to_nodes_defined.end()) {
                v = temp_variable_name_to_nodes_defined[temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            (temp_node->used).insert(temp);
            // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
            temp_node->parent.insert(v[v.size() - 1]);
        }
    }
    if (!control_dependence.empty()) {
        temp_node->parent.insert(control_dependence.top());
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
            for (auto it:v1) {
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
            for (auto it:v1) {
                if (it->flag) {
                    // cout<<curr<<" "<<v[i]->line_number<<" show output"<<endl;
                    q.push(it);
                }
            }
        }
    }
}

void find_affect_return_edge(struct node* function_calling_node, string function_name) {

    struct node* return_node = return_statements[function_map[function_name]];

    // cout<<function_name<<endl;
    // if(function_map.find(function_name) != function_map.end()) {
    //     cout<<"exist "<<function_name<<endl;
    // }
    // if(return_statements.find(function_map[function_name]) != return_statements.end()) {
    //     cout<<"exist in return statement "<<function_name<<endl;
    //     cout<<return_node->statement<<endl;
    // }

    queue<struct node*> q;
    // to mark visited nodes
    q.push(return_node);
    // cout<<"in affect return\n";
    while (!q.empty()) {
        struct node* temp_node = q.front();
        temp_node->flag = true;
        q.pop();
        unordered_set<struct node*> v1 = temp_node->parent;
        for (auto it:v1) {
            if (!it->flag) {
                // cout<<v1[j]->line_number<<" -> "<<v1[j]->statement<<endl;
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
        for (auto it:v1) {
            if (it->flag) {
                // cout<<curr<<" "<<v[i]->line_number<<" show output"<<endl;
                q.push(it);
            }
        }
    }
}

void process_function_call(string line, string number, int currentIndex, bool return_value_expects) {
    struct node* temp_node = new node(number, line); // node of function

    int i = 0;
    if (return_value_expects) {
        while (i < line.length() && line[i] != '=') {
            i++;
        }
    }
    // to find name of function
    pair<int, string> temp_token = find_token(i, line);
    string function_name = temp_token.second;

    // in case of library function call
    if(function_map.find(function_name) == function_map.end()) {
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
            if(in_thread) {
                temp_variable_name_to_nodes_defined[temp].push_back(temp_node);
            } else {
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
                vector<struct node*> v;
                if (temp_variable_name_to_nodes_defined.find(temp) != temp_variable_name_to_nodes_defined.end()) {
                    v = temp_variable_name_to_nodes_defined[temp];
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

        if (((int)temp[0] >= 48 && (int)temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'' || temp == " ") {
            continue;
        }
        else {
            // data dependence
            vector<struct node*> v;
            if (temp_variable_name_to_nodes_defined.find(temp) != temp_variable_name_to_nodes_defined.end()) {
                v = temp_variable_name_to_nodes_defined[temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            temp_node->parent.insert(v[v.size() - 1]);

            string temp1 = temp + "_in";
            struct node* actual_in_node = new node(number, temp1 + " = " + temp);
            actual_in_node->parent.insert(temp_node);

            if(in_thread) {
                temp_variable_name_to_nodes_defined[temp1].push_back(actual_in_node);
                temp_variable_name_to_nodes_used[temp].push_back(temp_node);
            } else {
                variable_name_to_nodes[temp1].push_back(actual_in_node);
            }
            (actual_in_node->used).insert(temp);
            (actual_in_node->defined).insert(temp1);
            temp_node->actual_in_nodes.push_back(actual_in_node);
            // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
            actual_in_node->parent.insert(temp_node);
            if (i - temp.length() - 1 >= 0 && line[i - temp.length() - 1] == '&') {
                temp1 = temp + "_out";
                struct node* actual_out_node = new node(number, temp + " = &" + temp1);
                actual_out_node->parent.insert(temp_node);
                
                if(in_thread) {
                    temp_variable_name_to_nodes_defined[temp].push_back(actual_out_node);
                    temp_variable_name_to_nodes_used[temp1].push_back(temp_node);
                } else {
                    variable_name_to_nodes[temp].push_back(actual_out_node);
                }

                (actual_out_node->used).insert(temp1);
                (actual_out_node->defined).insert(temp);
                // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
                temp_node->actual_out_nodes.push_back(actual_out_node);
                actual_out_node->parent.insert(temp_node);
            }
            // for(int j=0;j<v.size();j++) {
            //     temp_node->parent.push_back(v[j]);
            // }
        }
    }

    // for parameter edge
    if(temp_node->actual_in_nodes.size() > 0) {
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
    if (return_statements.find(function_map[function_name]) != return_statements.end()) {
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
        if(in_thread) {
            temp_variable_name_to_nodes_defined[temp].push_back(temp_node);
        } else {
            variable_name_to_nodes[temp].push_back(temp_node);
        }
    }

    //control dependence
    if (!control_dependence.empty()) {
        temp_node->parent.insert(control_dependence.top());
    }
    code[number] = temp_node;
}

void process_function_definition(string line, string number, int currentIndex) {
    variable_name_to_nodes.clear();                        /////////      ********       Alert!!!!!!!!
    struct node* temp_node = new node(number, line); // node of function
    // control_dependence.push(temp_node);
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

            // if(in_thread) {
            //     temp_variable_name_to_nodes_defined[temp].push_back(formal_in_node);
            //     temp_variable_name_to_nodes_used[temp1].push_back(temp_node);
            // } else {
                variable_name_to_nodes[temp].push_back(formal_in_node);
            // }

            (formal_in_node->used).insert(temp1);
            (formal_in_node->defined).insert(temp);
            temp_node->formal_in_nodes.push_back(formal_in_node);
            // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
            formal_in_node->parent.insert(temp_node);
            if (i - temp.length() - 1 >= 0 && line[i - temp.length() - 1] == '*') {
                temp1 = temp + "_out";
                struct node* formal_out_node = new node(number, temp1 + " = *" + temp);
                formal_out_node->parent.insert(temp_node);

                // if(in_thread) {
                //     temp_variable_name_to_nodes_defined[temp1].push_back(formal_out_node);
                //     temp_variable_name_to_nodes_used[temp].push_back(temp_node);
                // } else {
                    variable_name_to_nodes[temp1].push_back(formal_out_node);
                // }

                (formal_out_node->used).insert(temp1);
                (formal_out_node->defined).insert(temp1);
                // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
                temp_node->formal_out_nodes.push_back(formal_out_node);
                formal_out_node->parent.insert(temp_node);
            }
            // for(int j=0;j<v.size();j++) {
            //     temp_node->parent.push_back(v[j]);
            // }
        }
    }

    // to find name of function
    pair<int, string> p = find_token(0, line);
    p = find_token(p.first, line);
    function_map[p.second] = temp_node;
    code[number] = temp_node;
    // cout<<number<<" "<<temp_node->statement<<" in fun definition"<<endl;
}

void process_return(string line, int currentIndex, string number) {
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    while (i < line.length()) {
        pair<int, string> token = find_token(i, line);

        i = token.first;
        string temp = token.second;
        if (in_thread && !(temp[0] >= 48 && temp[0] <= 57) && temp != "" && temp[0] != '"' && temp[0] != '\'') {
            temp_variable_name_to_nodes_used[temp].push_back(temp_node);
        }
        if ((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'' || temp == "endl") {
            continue;
        }
        else if (variable_name_to_nodes.find(temp) == variable_name_to_nodes.end() && temp_variable_name_to_nodes_defined.find(temp) == temp_variable_name_to_nodes_defined.end()) {
            cout << "The variable name " << temp << " does not exist in cout portion\n";
        }
        else {
            vector<struct node*> v;
            if (temp_variable_name_to_nodes_defined.find(temp) != temp_variable_name_to_nodes_defined.end()) {
                v = temp_variable_name_to_nodes_defined[temp];
            }
            else {
                v = variable_name_to_nodes[temp];
            }
            (temp_node->used).insert(temp);
            // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
            temp_node->parent.insert(v[v.size() - 1]);
            // for(int j=0;j<v.size();j++) {
            //     temp_node->parent.push_back(v[j]);
            // }
        }
    }

    if (!control_dependence.empty()) {
        temp_node->parent.insert(control_dependence.top());
    }
    code[number] = temp_node;
    // cout<<control_dependence.top()->statement<<" in return processing\n";
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
        if (temp_variable_name_to_nodes_defined.find(temp) != temp_variable_name_to_nodes_defined.end()) {
            v1 = temp_variable_name_to_nodes_defined[temp];
        }
        else {
            v1 = variable_name_to_nodes[temp];
        }
        v[i]->parent.insert(v1[v1.size() - 1]);
    }
    // control_dependence.pop();
}

void find_shared_variables() {
    for (auto it = variable_name_to_nodes.begin(); it != variable_name_to_nodes.end();it++) {
        if (thread_private_variables.find(it->first) == thread_private_variables.end()) {
            shared_variables.insert(it->first);
        }
    }
}

void find_interference_edges() {
    for(int i=0;i<threads_variable_name_to_nodes_used.size();i++) {          // running over all of the threads
        unordered_map<string, vector<struct node*>> mp = threads_variable_name_to_nodes_used[i];
        for(auto it:shared_variables) {        // for to run over all shared variables
            if(mp.find(it) != mp.end()) {      // to check shared variable is used in the thread or not
                vector<struct node*> temp1 = mp[it];
                for(int j=0;j<threads_variable_name_to_nodes_used.size();j++) {   // run over all later threads
                    if(i == j) {
                        continue;
                    }
                    unordered_map<string, vector<struct node*>> mp1 = threads_variable_name_to_nodes_defined[j];
                    if(mp1.find(it) != mp1.end()) {   // to check that shared variable is defined inside another thread
                        vector<struct node*> temp2 = mp1[it];
                        for(int k=0;k<temp1.size();k++) {     // run over all used nodes
                            for(int l=0;l<temp2.size();l++) {  // run over all defined nodes
                                temp1[k]->interference_edge.push_back(temp2[l]);  // make interference edge between used -> defined nodes
                            }
                        }
                    }
                }
            }
        }
    }
}

void clone_section() {
    replication_in_progress = 1;
    stringstream ss1(cloning_ends.first);
    stringstream ss2(cloning_ends.second);
    string temp1,temp2;
    getline(ss1, temp1, '#');
    getline(ss2, temp2, '#');
    int low = stoi(temp1) + 2;
    int high = stoi(temp2);
    for(int i=low;i<high;i++) {
        process(source_code[i], to_string(i));
    }
    replication_in_progress = 0;
}

void process_pragma(string line, int currentIndex, string number) {
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    stringstream ss(number);
    string temp1;
    getline(ss, temp1, '#');
    // temp_node->in_thread = true;
    pair<int, string> token = find_token(i, line);
    i = token.first;
    while (i < line.length()) {
        token = find_token(i, line);
        i = token.first;
        string temp = token.second;
        if (temp == "parallel") {
            find_shared_variables();
            cloning_ends.first = number;
        }
        if (temp == "section") {
            // to add marking line for section
            marking_lines["section"].push_back(stoi(temp1));

            has_section = 1;
            thread_count_increment = 1;
            // to remove eralier section from control dependence
            string temp = control_dependence.top()->statement;
            pair<int,string> p = find_token(0, temp);
            p = find_token(p.first, temp);
            p = find_token(p.first, temp);
            if(p.second == "section") {
                thread_count++;
                control_dependence.pop();
            }

            // before clearing temp_variable_name_to_node, inserting shared variables into main variable_name_to_node
            if(!temp_variable_name_to_nodes_defined.empty()) {
                for(auto it:shared_variables) {
                    if(temp_variable_name_to_nodes_defined.find(it) != temp_variable_name_to_nodes_defined.end()) {
                        vector<struct node*> v = temp_variable_name_to_nodes_defined[it];
                        for(int i=0;i<v.size();i++) {
                            variable_name_to_nodes[it].push_back(v[i]);
                        }
                    }
                }
            }

            if(!temp_variable_name_to_nodes_defined.empty() || !temp_variable_name_to_nodes_used.empty()) {
                threads_variable_name_to_nodes_defined.push_back(temp_variable_name_to_nodes_defined);
                temp_variable_name_to_nodes_defined.clear();
                threads_variable_name_to_nodes_used.push_back(temp_variable_name_to_nodes_used);
                temp_variable_name_to_nodes_used.clear();
            }
        }
        if (temp == "parallel" || temp == "for") {
            temp_node->replicate = 1;
        }
        else if (temp == "sections" || temp == "section" || temp == "single" || temp == "master") {
            if(temp == "single" || temp == "master") {
                has_single_or_master = 1;
            } else if(temp == "sections") {
                dependent_lines["sections"].push_back(stoi(temp1));
            }
            temp_node->replicate = 0;
        }
        else if (temp == "shared") {
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
        temp_node->parent.insert(control_dependence.top());
    }
    // control_dependence.push(temp_node);
    code[number] = temp_node;
}

void resolve_thread_variables(string number, bool flag) {
    // before clearing temp_variable_name_to_node, inserting shared variables into main variable_name_to_node
    if(!temp_variable_name_to_nodes_defined.empty()) {
        for(auto it:shared_variables) {
            // cout<<it<<" shared variable\n";
            if(temp_variable_name_to_nodes_defined.find(it) != temp_variable_name_to_nodes_defined.end()) {
                // cout<<it<<" shared variable inserted\n";
                vector<struct node*> v = temp_variable_name_to_nodes_defined[it];
                for(int i=0;i<v.size();i++) {
                    variable_name_to_nodes[it].push_back(v[i]);
                }
            }
        }
    }
    
    if(!temp_variable_name_to_nodes_defined.empty() || !temp_variable_name_to_nodes_used.empty()) {
        threads_variable_name_to_nodes_defined.push_back(temp_variable_name_to_nodes_defined);
        temp_variable_name_to_nodes_defined.clear();
        threads_variable_name_to_nodes_used.push_back(temp_variable_name_to_nodes_used);
        temp_variable_name_to_nodes_used.clear();
    }

    if(thread_count_increment) {
        thread_count++;
    }

    if(!(has_section || has_single_or_master) && (flag || control_dependence.top()->replicate)) {
        if(!flag) {
            in_thread = 1;
        }
        parallel_bracket_count++;
        thread_count++;
        clone_section();
        thread_count = 0;
        parallel_bracket_count--;
        if(!flag) {
            in_thread = 0;
        }
        if(!temp_variable_name_to_nodes_defined.empty()) {
            for(auto it:shared_variables) {
                if(temp_variable_name_to_nodes_defined.find(it) != temp_variable_name_to_nodes_defined.end()) {
                    vector<struct node*> v = temp_variable_name_to_nodes_defined[it];
                    for(int i=0;i<v.size();i++) {
                        if (variable_name_to_nodes.find(it) != variable_name_to_nodes.end()) {
                            // cout<<it<<" reached\n";
                            vector<struct node*> v1 = variable_name_to_nodes[it];
                            variable_name_to_nodes[it].push_back(v[i]);
                            v[v.size() - 1]->parallel_thread_nodes.push_back(v1[v1.size()-1]);
                            // cout<<v[v.size()-1]->line_number<<" -> "<<v1[v1.size()-1]->line_number<<endl;
                        } else {
                            variable_name_to_nodes[it].push_back(v[i]);
                        }
                    }
                }
            }
        }

        if(!temp_variable_name_to_nodes_defined.empty() || !temp_variable_name_to_nodes_used.empty()) {
            threads_variable_name_to_nodes_defined.push_back(temp_variable_name_to_nodes_defined);
            temp_variable_name_to_nodes_defined.clear();
            threads_variable_name_to_nodes_used.push_back(temp_variable_name_to_nodes_used);
            temp_variable_name_to_nodes_used.clear();
        }
    }

    // for thread_private variables
    if (thread_private_variables.size() > 0) {
        for (auto it : thread_private_variables) {
            // cout << it << endl;
            if (threads_variable_name_to_nodes_defined[0].find(it) != threads_variable_name_to_nodes_defined[0].end()) {
                // cout << "in thread private variable : " << number << endl;
                vector<struct node*> v1 = threads_variable_name_to_nodes_defined[0][it];
                for (int i = 0; i < v1.size(); i++) {
                    variable_name_to_nodes[it].push_back(v1[i]);
                }
            }
        }
    }

    if(flag || control_dependence.top()->replicate) {
        find_interference_edges();         
        threads_variable_name_to_nodes_defined.clear();
        threads_variable_name_to_nodes_used.clear();
        temp_variable_name_to_nodes_defined.clear();
        temp_variable_name_to_nodes_used.clear();
    }
}

void process(string line, string number) {
    if(in_thread) {
        number += "#" + to_string(thread_count);
    }
    int i = 0;
    while(i < line.length() && line[i] == ' ') {
        i++;
    }
    if(i < line.length() && line[i] == '/' && i+1 < line.length() && line[i+1] == '/') {
        return;
    }

    i = 0;
    pair<int, string> token = find_token(i, line);
    i = token.first;
    string temp = token.second;
    // cout<<line<<" : "<<number<<" : "<<temp<<endl;
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
            i = 0;
            pair<int,string> p;
            while(i < line.length()) {
                p = find_token(i, line);
                i = p.first;
                if (p.second == "threadprivate") {
                    while (i < line.length()) {
                        token = find_token(i, line);
                        i = token.first;
                        if (token.second != "") {
                            shared_variables.erase(token.second);
                            thread_private_variables.insert(token.second);
                        }
                    }
                    break;
                } else if(p.second == "barrier") {
                    // cout<<"reached "<<number<<endl;
                    stringstream ss(number);
                    string temp1, temp2 = "";
                    getline(ss, temp1, '#');
                    cloning_ends.second = number;
                    resolve_thread_variables(number, true);
                    cloning_ends.first = to_string(stoi(temp1) - 1);
                    if(temp2 != "") {
                        cloning_ends.first += "#" + temp2;
                    }
                    cloning_ends.second = "0";
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
            if(in_thread) {
                parallel_bracket_count++;
            }
            string temp2, temp3;
            stringstream temp2_line(number);
            getline(temp2_line, temp2, '#');
            pair<int, string> p = find_token(0, source_code[stoi(temp2) - 1]);
            if (p.second == "while" || p.second == "for") { // to check that it is loop and not if else statement
                // getline(temp2_line, temp3, '_');
                // string temp4 = to_string(stoi(temp2) - 1) + '_' + temp3;
                string temp4 = to_string(stoi(temp2) - 1);
                if(in_thread) {
                    getline(temp2_line, temp3, '#');
                    temp4 += "#" + temp3;
                }
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
                    temp_node->parent.insert(control_dependence.top());
                }
                control_dependence.push(temp_node);
            }
            else if (p.second == "if" || p.second == "else") {
                string temp4 = to_string(stoi(temp2) - 1);
                if(in_thread) {
                    getline(temp2_line, temp3, '#');
                    temp4 += "#" + temp3;
                }
                control_dependence.push(code[temp4]);
            }
            else if (p.second == "pragma") {
                if(!in_thread) {
                    parallel_bracket_count++;
                    in_thread = 1;
                }
                string temp4 = to_string(stoi(temp2) - 1);
                if(in_thread) {
                    if(getline(temp2_line, temp3, '#')) {
                        temp4 += "#" + temp3;
                    }
                }
                process_pragma(source_code[stoi(temp2) - 1], p.first, temp4);
                control_dependence.push(code[temp4]);
            }
            else {
                // control_dependence.push(code[to_string(stoi(temp2) - 1)]);
                string temp4 = to_string(stoi(temp2) - 1);
                if(in_thread) {
                    getline(temp2_line, temp3, '#');
                    temp4 += "#" + temp3;
                }
                control_dependence.push(code[temp4]);
            }
            // cout<<control_dependence.top()->line_number<<" : "<<control_dependence.top()->statement<<" pushed\n";
            return;
        }
        if (temp1 == "}") {
            if(in_thread) {
                parallel_bracket_count--;
            }
            pair<int, string> p = find_token(0, control_dependence.top()->statement);
            if(p.second == "pragma") {
                if(in_thread && parallel_bracket_count == 0) {
                    in_thread = 0;
                }
                p = find_token(p.first, control_dependence.top()->statement);
                p = find_token(p.first, control_dependence.top()->statement);
                if(p.second == "sections") {
                    thread_count_increment = 0;
                    thread_count = 0;
                }
                // struct node* temp_node = new node(line, number);
                // cout<<number<<" line reached : "<<temp_variable_name_to_nodes_defined.size()<<endl;
                cloning_ends.second = number;
                resolve_thread_variables(number, false);
                if(!in_thread) {
                    thread_count = 0;
                }

                // temp_node->parent.push_back(control_dependence.top());

                // code[number] = temp_node;
                // cout<<control_dependence.top()->line_number<<" : "<<control_dependence.top()->statement<<" poped\n";
                control_dependence.pop();
                p = find_token(0, control_dependence.top()->statement);
                if(p.second != "pragma") {
                    cloning_ends = {"0","0"};
                    thread_private_variables.clear();
                    threads_variable_name_to_nodes_defined.clear();
                    threads_variable_name_to_nodes_used.clear();
                    temp_variable_name_to_nodes_defined.clear();
                    temp_variable_name_to_nodes_used.clear();
                }
                return;
            }

            //  && (while_loop_flag || for_loop_flag)) { // for to mark loop is over
            if (while_loop_flag)
                while_loop_flag = 0;
            if (for_loop_flag)
                for_loop_flag = 0;
            if (control_dependence.size() == 1) {
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
                    if (temp_variable_name_to_nodes_defined.find(temp) != temp_variable_name_to_nodes_defined.end()) {
                        v1 = temp_variable_name_to_nodes_defined[temp];
                    }
                    else {
                        v1 = variable_name_to_nodes[temp];
                    }
                    v[i]->parent.insert(v1[v1.size() - 1]);
                }
            }
            // cout<<control_dependence.top()->line_number<<" : "<<control_dependence.top()->statement<<" poped\n";
            control_dependence.pop();
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

bool cmp(struct node* a, struct node* b) {
    return a->line_number < b->line_number;
}

void show_output(string line_number, vector<string> variable_names) {
    struct node* temp_node = code[line_number];
    // for(int i=0;i<temp_node->parent.size();i++) {
    //     cout<<temp_node->parent[i]->line_number<<" ";
    // }
    // for(auto it=variable_name_to_nodes.begin();it!=variable_name_to_nodes.end();it++) {
    //     cout<<it->first<<endl;
    //     vector<struct node*> v = it->second;
    //     for(int i=0;i<v.size();i++) {
    //         struct node* temp = v[i];
    //         cout<<temp->line_number<<" ";
    //     }
    //     cout<<endl;
    // }
    // cout<<temp_node->mark<<" "<<temp_node->line_number<<endl;

    unordered_set<struct node*> ans;
    queue<struct node*> q;

    for(int i=0;i<variable_names.size();i++) {
        if(temp_node->defined.find(variable_names[i]) != temp_node->defined.end()) {
            q.push(temp_node);
            break;
        }
    }
    if(q.empty()) {
        for(int i=0;i<variable_names.size();i++) {
            if(temp_node->used.find(variable_names[i]) != temp_node->used.end()) {
                unordered_set<struct node*> parent = temp_node->parent;
                for(auto it:parent) {
                    if(it->defined.find(variable_names[i]) != it->defined.end()) {
                        q.push(it);
                    }
                }
            } else {
                cout<<"Variable name : "<<variable_names[i]<<" does not exist in line number "<<line_number<<endl;
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
            getline(ss1, s1, '#');
            getline(ss2, s2, '#');
            if (s1 == s2) {
                q.push(v1[i]);
            }
        }

        ans.insert(curr);
        q.pop();
        // cout<<curr<<" output\n";
        unordered_set<struct node*> v = curr->parent;

        //to find control and data dependent nodes
        for (auto it:v) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<it->statement<<" show output control, data"<<endl;
            // if(v[i]->mark && ans.find(v[i]) == ans.end()) {
            if (ans.find(it) == ans.end()) {
                q.push(it);
            }
        }

        //to find interference dependence nodes
        v1 = curr->interference_edge;
        for (int i = 0; i < v1.size(); i++) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<v1[i]->statement<<" show output intereference"<<endl;
            if (ans.find(v1[i]) == ans.end()) {
                q.push(v1[i]);
            }
        }

        // to find parameter in nodes
        if (curr->parametere_in_edge != NULL && ans.find(curr->parametere_in_edge) == ans.end()) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<curr->parametere_in_edge->statement<<" show output parameter in"<<endl;
            q.push(curr->parametere_in_edge);
        }

        // to find calling nodes
        if (curr->calling_edge != NULL && ans.find(curr->calling_edge) == ans.end()) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<curr->calling_edge->statement<<" show output calling"<<endl;
            q.push(curr->calling_edge);
        }

        // to find transitive dependent nodes
        v1 = curr->transitive_edge;
        for (int i = 0; i < v1.size(); i++) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<v1[i]->statement<<" show output transitive"<<endl;
            if (ans.find(v1[i]) == ans.end()) {
                q.push(v1[i]);
            }
        }

        //affect return edges
        v1 = curr->affect_return_edge;
        for (int i = 0; i < v1.size(); i++) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<v1[i]->statement<<" show output affect return"<<endl;
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
            getline(ss1, s1, '#');
            getline(ss2, s2, '#');
            if (s1 == s2) {
                q.push(v1[i]);
                ans.insert(v1[i]);
            }
        }

        // cout<<curr<<" output\n";
        unordered_set<struct node*> v = curr->parent;

        //to find control and data dependent nodes
        for (auto it:v) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<it->statement<<" show output control, data"<<endl;
            // if(v[i]->mark && ans.find(v[i]) == ans.end()) {
            if (ans.find(it) == ans.end()) {
                ans.insert(it);
                q.push(it);
            }
        }

        //to find interference dependence nodes
        v1 = curr->interference_edge;
        for (int i = 0; i < v1.size(); i++) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<v1[i]->statement<<" show output intereference"<<endl;
            if (ans.find(v1[i]) == ans.end()) {
                q.push(v1[i]);
                ans.insert(v1[i]);
            }
        }

        // to find parameter out nodes
        if (curr->parametere_out_edge != NULL && ans.find(curr->parametere_out_edge) == ans.end()) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<curr->parametere_out_edge->statement<<" show output parameter out"<<endl;
            ans.insert(curr->parametere_out_edge);
            q.push(curr->parametere_out_edge);
        }

        // to find transitive dependent nodes
        v1 = curr->transitive_edge;
        for (int i = 0; i < v1.size(); i++) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<v1[i]->statement<<" show output transitive"<<endl;
            if (ans.find(v1[i]) == ans.end()) {
                ans.insert(v1[i]);
                q.push(v1[i]);
            }
        }

        // affect return nodes
        v1 = curr->affect_return_edge;
        for (int i = 0; i < v1.size(); i++) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<v1[i]->statement<<" show output affect return"<<endl;
            if (ans.find(v1[i]) == ans.end()) {
                ans.insert(v1[i]);
                q.push(v1[i]);
            }
        }

        // return link edge
        if (curr->return_link != NULL && ans.find(curr->return_link) == ans.end()) {
            // cout<<curr->line_number<<" : "<<curr->statement<<" -> "<<curr->return_link->statement<<" show output return link"<<endl;
            stringstream ss(curr->statement);
            string temp;
            getline(ss, temp, '=');
            if(getline(ss, temp, '=')) {
                ans.insert(curr->return_link);
                q.push(curr->return_link);
            }
        }
    }

    unordered_set<string> temp_line_numbers;

    for (auto it : ans) {
        // cout<<it->line_number<<" : "<<it->statement<<endl;
        temp_line_numbers.insert(it->line_number);
    }

    // for sections line
    // vector<int> v1 = dependent_lines["section"];
    // vector<int> v2 = marking_lines["section"];
    // for (int i = 0; i < v2.size(); i++) {
    //     string temp1 = to_string(v2[i]) + "#0";
    //     string temp2 = to_string(v2[i]) + "#1";
    //     string temp3 = to_string(v2[i]) + "#2";
    //     string temp4 = to_string(v2[i]) + "#3";
    //     if (temp_line_numbers.find(temp1) != temp_line_numbers.end() || 
    //         temp_line_numbers.find(temp2) != temp_line_numbers.end() || 
    //         temp_line_numbers.find(temp3) != temp_line_numbers.end() || 
    //         temp_line_numbers.find(temp4) != temp_line_numbers.end()) {
    //         int last = 0;
    //         for (int j = 0; j < v1.size(); j++) {
    //             if (v1[j] > v2[i]) {
    //                 break;
    //             }
    //             last = v1[j];
    //         }
    //         temp_line_numbers.insert(to_string(last));
    //     }
        // struct node* temp_node;
        // if(code.find(temp1) != code.end()) {
        //     temp_node = code[temp1];
        //     cout<<temp1<<endl;
        // } else {
        //     temp_node = code[temp2];
        //     cout<<temp2<<endl;
        // }

        // unordered_set<node*> v = temp_node->parent;
        
        // for(auto it:v) {
        //     cout<<it->line_number<<endl;
        //     if(code.find())
        // }
        // cout<<endl;
    // }

    // for 'for' line
    vector<int> v1 = dependent_lines["for"];
    vector<int> v2 = marking_lines["for"];
    for (int i = 0; i < v2.size(); i++) {
        string temp = to_string(v2[i]) + "#0";
        if (temp_line_numbers.find(temp) != temp_line_numbers.end()) {
            int last = 0;
            for (int j = 0; j < v1.size(); j++) {
                if (v1[j] > v2[i]) {
                    break;
                }
                last = v1[j];
            }
            struct node* temp_node = new node(to_string(last), source_code[last]);
            
            struct node* temp1 = code[temp];
            unordered_set<node*> v = temp1->parent;
            for(auto it:v) {
                pair<int,string> p = find_token(0, it->statement);
                if(p.second == "if" || p.second == "else" || p.second == "pragma") {
                    temp_node->parent.insert(it);
                    temp1->parent.erase(it);
                    temp1->parent.insert(temp_node);
                }
            }
            code[to_string(last)] = temp_node;
            // temp_line_numbers.insert(to_string(last));
            ans.insert(temp_node);
        }
    }

    vector<struct node*> ans_temp;
    for (auto it : ans) {
        ans_temp.push_back(it);
    }

    sort(ans_temp.begin(), ans_temp.end(), cmp);

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
    slice.close();
    CD.close();
    T.close();
    I.close();
    AR.close();
    CE.close();
    PI.close();
    PO.close();
    RL.close();
    // for (int j = 0; j < ans_temp.size(); j++) {
    //     if(code.find(ans_temp[j]) != code.end()) {
    //         cout << ans_temp[j] << " : " << code[ans_temp[j]]->statement <<" : "<< code[ans_temp[j]]->transitive_edge.size() << "\n";
    //     } else {
    //         stringstream ss(ans_temp[j]);
    //         string s1;
    //         getline(ss, s1, '#');
    //         cout << ans_temp[j] << " : " << source_code[stoi(s1)] << "\n";
    //     }
    // }
}

void add_lines_from_header_file(string temp) {
    ifstream fin;
    fin.open(temp);
    string line;
    while(!fin.eof()) {
        total_number_of_lines_of_header_files++;
        getline(fin, line);
        int i = 1;
        while(i < line.length() && line[i] == ' ') {
            i++;
        }
        string temp = "";
        while(i < line.length() && line[i] >= 97 && line[i] <= 122) {
            temp += line[i++];
        }

        if(temp == "include") {
            i++;
            temp = "";
            while(i < line.length() && ((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90) || line[i] == '.')) {
                temp += line[i++];
            }
            ifstream fin2;
            fin2.open(temp);
            if(fin2.is_open()) {
                fin2.close();
                add_lines_from_header_file(temp);
            } else {
                output<<line + "\n";
            }
        } else {
            output<<line + "\n";
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
    output.open("output.cpp");
    string slicing_line_number;
    int num;
    // cout << "Enter line number where you wnat to find slice : ";
    cin >> slicing_line_number;
    // cout << "Enter number of variables : ";
    cin >> num;
    vector<string> variable_names(num);
    for(int i=0;i<num;i++) {
        // cout << "Enter variable name : ";
        cin >> variable_names[i];
    }
    // cout << "Source file parsing is in progress...\n";
    int line_count = 0;

    // to add lines of source code to output file which is helpful in case of importing functions from header files
    while (!fin.eof()) {
        getline(fin, line);
        int i = 1;
        while(i < line.length() && line[i] == ' ') {
            i++;
        }
        string temp = "";
        while(i < line.length() && line[i] >= 97 && line[i] <= 122) {
            temp += line[i++];
        }

        if(temp == "include") {
            i++;
            temp = "";
            while(i < line.length() && ((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90) || line[i] == '.')) {
                temp += line[i++];
            }
            ifstream fin2;
            fin2.open(temp);
            if(fin2.is_open()) {
                fin2.close();
                add_lines_from_header_file(temp);
            } else {
                output<<line + "\n";
            }
        } else {
            output<<line + "\n";
        }
    }

    fin.close();
    output.close();

    ifstream fin2;
    fin2.open("output.cpp");
    while (!fin2.eof()) {
        getline(fin2, line);
        source_code.push_back(line);
        line_count++;
        // cout<<line_count<<" -> "<<line<<endl;
        // cout<<line_count<<" : "<<temp_variable_name_to_nodes_defined.size()<<" : "<<temp_variable_name_to_nodes_used.size()<<endl;
        process(line, to_string(line_count));
        // cout<<line_count<<" : "<<temp_variable_name_to_nodes_defined.size()<<" : "<<temp_variable_name_to_nodes_used.size()<<endl;
        // cout<<line<<endl;
    }
    fin2.close();

    stringstream ss(slicing_line_number);
    string temp;
    getline(ss, temp, '_');
    string temp2 = "";
    temp = to_string(stoi(temp) + total_number_of_lines_of_header_files);
    if(getline(ss, temp2, '_')) {
        temp += "_" + temp2;
    }
    slicing_line_number = temp;
    
    if (code.find(slicing_line_number) == code.end() && 
        code.find(slicing_line_number + "#0") == code.end() &&
        code.find(slicing_line_number + "#1") == code.end() &&
        code.find(slicing_line_number + "#2") == code.end() &&
        code.find(slicing_line_number + "#3") == code.end()) {
        cout << "You entered line number does not contains \n";
        return 0;
    }
    if (code.find(slicing_line_number) != code.end()) {
        show_output(slicing_line_number, variable_names);
    } 
    else if (code.find(slicing_line_number + "#0") != code.end()) {
        show_output(slicing_line_number + "#0", variable_names);
    }
    else if (code.find(slicing_line_number + "#1") != code.end()) {
        show_output(slicing_line_number + "#1", variable_names);
    }
    else if (code.find(slicing_line_number + "#2") != code.end()) {
        show_output(slicing_line_number + "#2", variable_names);
    }
    else if (code.find(slicing_line_number + "#3") != code.end()) {
        show_output(slicing_line_number + "#3", variable_names);
    }

    remove("output.cpp");
    
}
