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
unordered_map<string, struct node*> function_map;
unordered_map<struct node*, struct node*> return_statements;
unordered_set<string> datatypes; // for inbuilt datatypes
stack<struct node*> control_dependence;
stack<string> control_dependence2; // for to create trace keep track of scope
string file_opening_line = "ofstream result;\nresult.open(\"result.txt\", ios::trunc);\nresult<<";
string for_main_file_opening_line = "ofstream result;\nresult.open(\"result.txt\", ios::app);\nresult<<";
vector<string> source_code; // to find out line by line_number directly
vector<int> conditional_statements; // to keep track of hierarchy of if else statements
bool while_loop_flag = 0, for_loop_flag = 0, function_starting = 0, condition_flag = 0;

struct node {
    vector<struct node*> parent; // contains control and data dependences
    struct node* parametere_in_edge, *parametere_out_edge, *calling_edge; // parameter IN/OUT edge
    struct node* return_link;
    vector<struct node*> transitive_edge, affect_return_edge;
    bool mark; // for dynamic dependence graph
    bool flag; // for transitive edge and affect return edge
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

    datatypes.insert("int");
    datatypes.insert("float");
    datatypes.insert("double");
    datatypes.insert("string");
    datatypes.insert("void");
}

//parse words from the line
pair<int,string> find_token(int i, string line) {
    string temp = "";
    while(i < line.length() && !(line[i] >= 97 && line[i] <= 122) && !(line[i] >= 65 && line[i] <= 90)
        && !(line[i] >= 48 && line[i] <= 57) && line[i] != '_' && line[i] != '"' && line[i] != '\'') {
            i++;
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
    while(getline(temp_line, temp, '=')) {
        tokens.push_back(temp);
    }
    if(tokens.size() < 2) {
        return;
    }

    struct node* temp_node;
    if(code.find(number) == code.end()) {
        temp_node = new node(number,line);
    } else {
        temp_node = code[number];
    }
    string variable_name;
    while(i < tokens[0].length()) {
        pair<int,string> token = find_token(i, tokens[0]);
        i = token.first;
        if(datatypes.find(token.second) == datatypes.end() && token.second.length() > 0)
            variable_name = token.second;
    }
    (temp_node->defined).insert(variable_name);
    i = 0;
    line = tokens[1];
    while(i < line.length()) {
        pair<int,string> token = find_token(i, line);
        
        i = token.first;
        string temp = token.second;
        if((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        } else if(variable_name_to_nodes.find(temp) == variable_name_to_nodes.end()) {
            cout<<"The variable name "<<temp<<" does not exist in definition portion\n";
        } else {
            (temp_node->used).insert(temp);
            vector<struct node*> v = variable_name_to_nodes[temp];
            temp_node->parent.push_back(v[v.size()-1]);
            // for(int j=0;j<v.size();j++) {
            //     temp_node->parent.push_back(v[j]);
            // }
        }
    }
    if(!control_dependence.empty()) {
        temp_node->parent.push_back(control_dependence.top());
    }
    variable_name_to_nodes[variable_name].push_back(temp_node);
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
    while(getline(temp1_line, temp1, ';')) {
        tokenized_line.push_back(temp1);
    }
    stringstream temp2_line(number);
    while(getline(temp2_line, temp1, '_')) {
        tokenized_number.push_back(temp1);
    }
    
    if(tokenized_number[1] == "0") {
        process_definition(tokenized_line[0], 0, number);
    }

    i = 0;
    while(i < tokenized_line[1].length()) {
        pair<int,string> token = find_token(i, tokenized_line[1]);
        
        i = token.first;
        string temp = token.second;
        if((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        } else if(variable_name_to_nodes.find(temp) == variable_name_to_nodes.end()) {
            cout<<"The variable name "<<temp<<" does not exist in if portion\n";
        } else {
            (temp_node->used).insert(temp);
            vector<struct node*> v = variable_name_to_nodes[temp];
            temp_node->parent.push_back(v[v.size()-1]);
            // for(int i=0;i<v.size();i++) {
            //     temp_node->parent.push_back(v[i]);
            // }
        }
    }
    // if(!control_dependence.empty()) {
    //     temp_node->parent.push_back(control_dependence.top());
    // }
    // control_dependence.push(temp_node);
}

void process_while(string line, int currentIndex, string number) {
    // loop_flag = 1;
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    while(i < line.length()) {
        pair<int,string> token = find_token(i, line);
        
        i = token.first;
        string temp = token.second;
        
        if((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        } else if(variable_name_to_nodes.find(temp) == variable_name_to_nodes.end()) {
            cout<<"The variable name "<<temp<<" does not exist in if portion\n";
        } else {
            (temp_node->used).insert(temp);
            vector<struct node*> v = variable_name_to_nodes[temp];
            temp_node->parent.push_back(v[v.size()-1]);
            // for(int i=0;i<v.size();i++) {
            //     temp_node->parent.push_back(v[i]);
            // }
        }
    }
    // if(!control_dependence.empty()) {
    //     temp_node->parent.push_back(control_dependence.top());
    // }
    // control_dependence.push(temp_node);
    code[number] = temp_node;
}

void process_if(string line, int currentIndex, string number) {
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    if(!control_dependence.empty()) {
        temp_node->parent.push_back(control_dependence.top());
        control_dependence.pop();
    }
    // control_dependence.push(temp_node);
    while(i < line.length()) {
        pair<int,string> token = find_token(i, line);
        
        i = token.first;
        string temp = token.second;
        
        if((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        } else if(variable_name_to_nodes.find(temp) == variable_name_to_nodes.end()) {
            cout<<"The variable name "<<temp<<" does not exist in if portion\n";
        } else {
            (temp_node->used).insert(temp);
            vector<struct node*> v = variable_name_to_nodes[temp];
            temp_node->parent.push_back(v[v.size()-1]);
            // for(int i=0;i<v.size();i++) {
            //     temp_node->parent.push_back(v[i]);
            // }
        }
    }
    // cout<<control_dependence.top()->line_number<<endl;
    code[number] = temp_node;
}

void process_else(string line, int currentIndex, string number) {
    int i = currentIndex;
    while(i < line.length()) {
        pair<int,string> token = find_token(i, line);
        
        i = token.first;
        string temp = token.second;

        if(temp == "if") {
            process_if(line, i, number);
            return;
        }
    }
    struct node* temp_node = new node(number, line);
    if(!control_dependence.empty()) {
        temp_node->parent.push_back(control_dependence.top());
        control_dependence.pop();
    }
    // control_dependence.push(temp_node);
    code[number] = temp_node;
}

void process_cin(string line, int currentIndex, string number) {
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    while(i < line.length()) {
        pair<int,string> token = find_token(i, line);
        
        i = token.first;
        string temp = token.second;
        if(temp == "") {
            continue;
        }
        (temp_node->defined).insert(temp);
        variable_name_to_nodes[temp].push_back(temp_node);
    }
    if(!control_dependence.empty()) {
        temp_node->parent.push_back(control_dependence.top());
    }
    code[number] = temp_node;
}

//not cosidered case of "variable_name"
void process_cout(string line, int currentIndex, string number) {
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    while(i < line.length()) {
        pair<int,string> token = find_token(i, line);
        
        i = token.first;
        string temp = token.second;

        if((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'' || temp == "endl") {
            continue;
        } else if(variable_name_to_nodes.find(temp) == variable_name_to_nodes.end()) {
            cout<<"The variable name "<<temp<<" does not exist in cout portion\n";
        } else {
            vector<struct node*> v = variable_name_to_nodes[temp];
            (temp_node->used).insert(temp);
            // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
            temp_node->parent.push_back(v[v.size()-1]);
            // for(int j=0;j<v.size();j++) {
            //     temp_node->parent.push_back(v[j]);
            // }
        }
    }
    if(!control_dependence.empty()) {
        temp_node->parent.push_back(control_dependence.top());
    }
    code[number] = temp_node;
}

void find_transitive_dependence_edge(struct node* function_calling_node, string function_name) {
    vector<struct node*> v = function_map[function_name]->formal_out_nodes;
    for(int i=0;i<v.size();i++) {
        queue<struct node*> q;
        
        // to mark visited nodes
        q.push(v[i]);
        while(!q.empty()) {
            struct node* temp_node = q.front();
            temp_node->flag = true;
            q.pop();
            vector<struct node*> v1 = temp_node->parent;
            for(int j=0;j<v1.size();j++) {
                if(!v1[j]->flag) {
                    q.push(v1[j]);
                }
            }
        }

        vector<struct node*> v2 = function_map[function_name]->formal_in_nodes;
        for(int j=0;j<v2.size();j++) {
            if(v2[j]->flag) {
                function_calling_node->actual_out_nodes[i]->transitive_edge.push_back(function_calling_node->actual_in_nodes[j]);
            }
        }

        // to unmark visited nodes
        q.push(v[i]);
        while(!q.empty()) {
            struct node* temp_node = q.front();
            temp_node->flag = false;
            q.pop();
            vector<struct node*> v1 = temp_node->parent;
            for(int j=0;j<v1.size();j++) {
                if(v1[j]->flag) {
                    // cout<<curr<<" "<<v[i]->line_number<<" show output"<<endl;
                    q.push(v1[j]);
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
    while(!q.empty()) {
        struct node* temp_node = q.front();
        temp_node->flag = true;
        q.pop();
        vector<struct node*> v1 = temp_node->parent;
        for(int j=0;j<v1.size();j++) {
            if(!v1[j]->flag) {
                // cout<<curr<<" "<<v[i]->line_number<<" show output"<<endl;
                q.push(v1[j]);
            }
        }
    }

    vector<struct node*> v2 = function_map[function_name]->formal_in_nodes;
    for(int j=0;j<v2.size();j++) {
        if(v2[j]->flag) {
            function_calling_node->affect_return_edge.push_back(function_calling_node->actual_in_nodes[j]);
        }
    }

    // to unmark visited nodes
    q.push(return_node);
    while(!q.empty()) {
        struct node* temp_node = q.front();
        temp_node->flag = false;
        q.pop();
        vector<struct node*> v1 = temp_node->parent;
        for(int j=0;j<v1.size();j++) {
            if(v1[j]->flag) {
                // cout<<curr<<" "<<v[i]->line_number<<" show output"<<endl;
                q.push(v1[j]);
            }
        }
    }
}

void process_function_call(string line, string number, int currentIndex, bool return_value_expects) {
    struct node* temp_node = new node(number, line); // node of function

    int i = currentIndex;
    while(i < line.length()) {
        pair<int,string> token = find_token(i, line);
        
        i = token.first;
        string temp = token.second;

        if((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'') {
            continue;
        } else {
            // data dependence
            temp_node->parent.push_back(variable_name_to_nodes[temp][variable_name_to_nodes[temp].size()-1]);

            string temp1 = temp + "_in";
            struct node* actual_in_node = new node(number, temp1 + " = " + temp);
            actual_in_node->parent.push_back(temp_node);
            variable_name_to_nodes[temp1].push_back(actual_in_node);
            (actual_in_node->used).insert(temp);
            (actual_in_node->defined).insert(temp1);
            temp_node->actual_in_nodes.push_back(actual_in_node);
            // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
            actual_in_node->parent.push_back(temp_node);
            if(i-temp.length()-1 >= 0 && line[i-temp.length()-1] == '&') {
                temp1 = temp + "_out";
                struct node* actual_out_node = new node(number, temp + " = &" + temp1);
                actual_out_node->parent.push_back(temp_node);
                variable_name_to_nodes[temp].push_back(actual_out_node);
                (actual_out_node->used).insert(temp1);
                (actual_out_node->defined).insert(temp);
                // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
                temp_node->actual_out_nodes.push_back(actual_out_node);
                actual_out_node->parent.push_back(temp_node);
            }
            // for(int j=0;j<v.size();j++) {
            //     temp_node->parent.push_back(v[j]);
            // }
        }
    }

    i = 0;
    if(return_value_expects) {
        while(i < line.length() && line[i] != '=') {
            i++;
        }
    }
    // to find name of function
    pair<int, string> p = find_token(i, line);
    string function_name = p.second;

    // for parameter edge
    vector<struct node*> v1 = function_map[function_name]->formal_in_nodes;
    vector<struct node*> v2 = temp_node->actual_in_nodes;
    for(int i=0;i<v1.size();i++) {
        v1[i]->parametere_in_edge = v2[i];
    }
    v1 = function_map[function_name]->formal_out_nodes;
    v2 = temp_node->actual_out_nodes;
    for(int i=0;i<v1.size();i++) {
        v2[i]->parametere_out_edge = v1[i];
    }

    // for calling edge
    function_map[function_name]->calling_edge = temp_node;

    // for transitive edge
    if(temp_node->actual_out_nodes.size() > 0) {
        find_transitive_dependence_edge(temp_node, function_name);
    }

    //return link edge
    if(return_statements.find(function_map[function_name]) != return_statements.end()) {
        struct node* return_node = return_statements[function_map[function_name]];
        temp_node->return_link = return_node;
    }

    //affect return edge
    if(return_value_expects) {
        find_affect_return_edge(temp_node, function_name);
    }
    
    //if return_value_expects than find which variable is defined
    if(return_value_expects) {
        int i = 0;
        while(i < line.length() && line[i] != '=') {
            i++;
        }
        while(i >= 0 && !((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90))) {
            i--;
        }
        string temp = "";
        while(i >= 0 && ((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90))) {
            temp = line[i--] + temp;
        }
        variable_name_to_nodes[temp].push_back(temp_node);
    }

    //control dependence
    if(!control_dependence.empty()) {
        temp_node->parent.push_back(control_dependence.top());
    }
    code[number] = temp_node;
}

void process_function_definition(string line, string number, int currentIndex) {
    variable_name_to_nodes.clear();                        /////////      ********       Alert!!!!!!!!
    struct node* temp_node = new node(number, line); // node of function
    // control_dependence.push(temp_node);
    int i = currentIndex;
    while(i < line.length()) {
        pair<int,string> token = find_token(i, line);
        
        i = token.first;
        string temp = token.second;

        if((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'' || datatypes.find(temp) != datatypes.end()) {
            continue;
        } else {
            string temp1 = temp + "_in";
            struct node* formal_in_node = new node(number, temp + " = " + temp1);
            formal_in_node->parent.push_back(temp_node);
            variable_name_to_nodes[temp].push_back(formal_in_node);
            (formal_in_node->used).insert(temp1);
            (formal_in_node->defined).insert(temp);
            temp_node->formal_in_nodes.push_back(formal_in_node);
            // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
            formal_in_node->parent.push_back(temp_node);
            if(i-temp.length()-1 >= 0 && line[i-temp.length()-1] == '*') {
                temp1 = temp + "_out";
                struct node* formal_out_node = new node(number, temp1 + " = *" + temp);
                formal_out_node->parent.push_back(temp_node);
                variable_name_to_nodes[temp1].push_back(formal_out_node);
                (formal_out_node->used).insert(temp1);
                (formal_out_node->defined).insert(temp1);
                // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
                temp_node->formal_out_nodes.push_back(formal_out_node);
                formal_out_node->parent.push_back(temp_node);
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
}

void process_return(string line, int currentIndex, string number) {
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    while(i < line.length()) {
        pair<int,string> token = find_token(i, line);
        
        i = token.first;
        string temp = token.second;

        if((temp[0] >= 48 && temp[0] <= 57) || temp == "" || temp[0] == '"' || temp[0] == '\'' || temp == "endl") {
            continue;
        } else if(variable_name_to_nodes.find(temp) == variable_name_to_nodes.end()) {
            cout<<"The variable name "<<temp<<" does not exist in cout portion\n";
        } else {
            vector<struct node*> v = variable_name_to_nodes[temp];
            (temp_node->used).insert(temp);
            // cout<<number<<" - "<<v[v.size()-1]->line_number<<" in cout"<<endl;
            temp_node->parent.push_back(v[v.size()-1]);
            // for(int j=0;j<v.size();j++) {
            //     temp_node->parent.push_back(v[j]);
            // }
        }
    }
    if(!control_dependence.empty()) {
        temp_node->parent.push_back(control_dependence.top());
    }
    code[number] = temp_node;
    return_statements[control_dependence.top()] = temp_node;

    // to set data depenedence for formal out node
    struct node* function_node = control_dependence.top();
    vector<struct node*> v = function_node->formal_out_nodes;
    for(int i=0;i<v.size();i++) {
        string temp = v[i]->statement;
        int j = 0;
        while(j < temp.length() && temp[j] != '*') {
            j++;
        }
        j++;
        temp = temp.substr(j);
        vector<struct node*> v1 = variable_name_to_nodes[temp];
        v[i]->parent.push_back(v1[v1.size()-1]);
    }
    control_dependence.pop();
}

void process(string line, string number) {
    int i = 0;
    pair<int,string> token = find_token(i, line);
    i = token.first;
    string temp = token.second;
    if(keywords.find(temp) != keywords.end()) {
        switch (keywords[temp]) {
            case 1:
                process_for(line,i,number); 
                break;
            case 2: 
                process_while(line,i,number);
                break;
            case 3:
                process_if(line,i,number);
                break;
            case 4:
                process_else(line,i,number);
                break;
            case 5:
                process_cin(line,i,number);
                break;
            case 6:
                process_cout(line,i,number);
                break;
            case 7:
                process_return(line,i,number);
                break;
        }
    } else {
        string temp1 = "";
        int j = 0;
        while(j < line.length() && line[j] == ' ') {
            j++;
        }
        while(j < line.length() && line[j] != ' ') {
            temp1 += line[j++];
        }
        if(temp1 == "{") { // to mark enterd into scope
            string temp2,temp3;
            stringstream temp2_line(number);
            getline(temp2_line, temp2, '_');
            pair<int, string> p = find_token(0, source_code[stoi(temp2)-1]);
            if(p.second == "while" || p.second == "for") { // to check that it is loop and not if else statement
                getline(temp2_line, temp3, '_');
                string temp4 = to_string(stoi(temp2) - 1) + '_' + temp3;
                if(p.second == "while")
                    while_loop_flag = 1;
                else {
                    string temp1;
                    vector<string> tokenized_line;
                    stringstream temp1_line(code[temp4]->statement);
                    while(getline(temp1_line, temp1, ';')) {
                        tokenized_line.push_back(temp1);
                    }
                    process_definition(tokenized_line[2].substr(0, tokenized_line[2].length()-1), 0, temp4);
                    // process for loops increamenting condition as this will execute only when we enter into loop
                    for_loop_flag = 1;
                }
                struct node* temp_node = code[temp4];
                if(!control_dependence.empty()) {
                    temp_node->parent.push_back(control_dependence.top());
                }
                control_dependence.push(temp_node);
            } else if(p.second == "if" || p.second == "else") {
                if(for_loop_flag || while_loop_flag) {
                    string temp2,temp3;
                    stringstream temp2_line(number);
                    getline(temp2_line, temp2, '_');
                    getline(temp2_line, temp3, '_');
                    string temp = to_string(stoi(temp2) - 1) + "_" + temp3;
                    control_dependence.push(code[temp]);
                } else {
                    string temp = to_string(stoi(temp2) - 1);
                    control_dependence.push(code[temp]);
                }
            } else {
                control_dependence.push(code[to_string(stoi(temp2) - 1)]);
            }
            return;
        }
        if(temp1 == "}") {
        //  && (while_loop_flag || for_loop_flag)) { // for to mark loop is over
            if(while_loop_flag)
                while_loop_flag = 0;
            if(for_loop_flag)
                for_loop_flag = 0;
            if(control_dependence.size() == 1) {
                struct node* function_node = control_dependence.top();
                vector<struct node*> v = function_node->formal_out_nodes;
                for(int i=0;i<v.size();i++) {
                    string temp = v[i]->statement;
                    int j = 0;
                    while(j < temp.length() && temp[j] != '*') {
                        j++;
                    }
                    j++;
                    temp = temp.substr(j);
                    vector<struct node*> v1 = variable_name_to_nodes[temp];
                    v[i]->parent.push_back(v1[v1.size()-1]);
                }
            }
            control_dependence.pop();
            return;
        }

        // to find if it is function call or definition or else
        j = 0;
        while(j < line.length() && line[j] == ' ') {
            j++;
        }
        temp1 = "";
        bool return_value_expects = 0;
        while(j < line.length() && line[j] != '(') {
            if(line[j] == '=') {
                return_value_expects = 1;
            }
            if(line[j] != '(') {
                temp1 += line[j];
            }
            j++;
        }
        if(j < line.length()) {
            if(return_value_expects) {
                process_function_call(line, number, j, 1);
            } else if(function_map.find(temp1) != function_map.end()) {
                process_function_call(line, number, j, 0);
            } else if(j < line.length()) {
                process_function_definition(line, number, j);
            } 
        } else {
            process_definition(line,0,number);
        }
    }
}

void find_trace() {
    string s = "g++ -o output output.cpp";
    const char *command = s.c_str();
    system(command);
    system("output.exe");

    ifstream fin;
    fin.open("result.txt");
    string temp;
    getline(fin, temp);
    string temp1;
    stringstream temp_line(temp);
    while(getline(temp_line, temp1, '#')) {
        string temp2;
        stringstream temp2_line(temp1);
        getline(temp2_line, temp2, '_');
        process(source_code[stoi(temp2)], temp1);
        if(code.find(temp1) != code.end()) {
            code[temp1]->mark = true;
        }
    }
}

// struct node* find_last_defined_node(pair<int,string> p) {
//     int temp_number = -1;
//     string variable_name = p.second;
//     int index = p.first;
//     vector<struct node*> v = variable_name_to_nodes[variable_name];
//     for(int i=0;i<v.size();i++) {
//         if(v[i]->line_number > temp_number && v[i]->line_number < index) {
//             temp_number = v[i]->line_number;
//         }
//     }
//     if(temp_number == -1) {
//         return NULL;
//     } else {
//         return code[temp_number];
//     }
// }

bool cmp(string a, string b) {
    string temp1, temp2;
    vector<int> a_temp, b_temp;
    stringstream temp1_line(a), temp2_line(b);
    while(getline(temp1_line, temp1, '_')) {
        a_temp.push_back(stoi(temp1));
    }
    while(getline(temp2_line, temp2, '_')) {
        b_temp.push_back(stoi(temp2));
    }
    if(a_temp.size() > 1 && b_temp.size() > 1 && a_temp[1] < b_temp[1])
        return 1;
    if(a_temp.size() > 1 && b_temp.size() > 1 && a_temp[1] > b_temp[1])
        return 0;
    if(a_temp[0] < b_temp[0]) {
        return 1;
    }
    return 0;
}

void show_output(string line_number, string variable_name) {
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

    // 1st phase
    q.push(temp_node);
    while(!q.empty()) {
        struct node* curr = q.front();
        ans.insert(curr);
        q.pop();
        // cout<<curr<<" output\n";
        vector<struct node*> v = curr->parent;
        
        //to find control and data dependent nodes
        for(int i=0;i<v.size();i++) {
            // cout<<curr->statement<<" -> "<<v[i]->statement<<" show output control, data"<<endl;
            // if(v[i]->mark && ans.find(v[i]) == ans.end()) {
            if(ans.find(v[i]) == ans.end()) {
                q.push(v[i]);
            }
        }

        // to find parameter in nodes
        if(curr->parametere_in_edge != NULL && ans.find(curr->parametere_in_edge) == ans.end()) {
            // cout<<curr->statement<<" -> "<<curr->parametere_in_edge->statement<<" show output parameter"<<endl;
            q.push(curr->parametere_in_edge);
        }

        // to find calling nodes
        if(curr->calling_edge != NULL && ans.find(curr->calling_edge) == ans.end()) {
            // cout<<curr->statement<<" -> "<<curr->calling_edge->statement<<" show output calling"<<endl;
            q.push(curr->calling_edge);
        }

        // to find transitive dependent nodes
        v = curr->transitive_edge;
        for(int i=0;i<v.size();i++) {
            // cout<<curr->statement<<" -> "<<v[i]->statement<<" show output transitive"<<endl;
            if(ans.find(v[i]) == ans.end()) {
                q.push(v[i]);
            }
        }

        //affect return edges
        v = curr->affect_return_edge;
        for(int i=0;i<v.size();i++) {
            // cout<<curr->statement<<" -> "<<v[i]->statement<<" show output affect return"<<endl;
            if(ans.find(v[i]) == ans.end()) {
                q.push(v[i]);
            }
        }
    }

    // cout<<"1st phase complete\n";

    // 2nd phase 
    // add all visited nodes
    for(auto it:ans) {
        q.push(it);
    }

    while(!q.empty()) {
        struct node* curr = q.front();
        q.pop();
        // cout<<curr<<" output\n";
        vector<struct node*> v = curr->parent;
        
        //to find control and data dependent nodes
        for(int i=0;i<v.size();i++) {
            // cout<<curr->statement<<" -> "<<v[i]->statement<<" show output control, data"<<endl;
            // if(v[i]->mark && ans.find(v[i]) == ans.end()) {
            if(ans.find(v[i]) == ans.end()) {
                ans.insert(v[i]);
                q.push(v[i]);
            }
        }

        // to find parameter out nodes
        if(curr->parametere_out_edge != NULL && ans.find(curr->parametere_out_edge) == ans.end()) {
            // cout<<curr->statement<<" -> "<<curr->parametere_out_edge->statement<<" show output parameter"<<endl;
            ans.insert(curr->parametere_out_edge);
            q.push(curr->parametere_out_edge);
        }

        // to find transitive dependent nodes
        v = curr->transitive_edge;
        for(int i=0;i<v.size();i++) {
            // cout<<curr->statement<<" -> "<<v[i]->statement<<" show output transitive"<<endl;
            if(ans.find(v[i]) == ans.end()) {
                ans.insert(v[i]);
                q.push(v[i]);
            }
        }

        // affect return nodes
        v = curr->affect_return_edge;
        for(int i=0;i<v.size();i++) {
            // cout<<curr->statement<<" -> "<<v[i]->statement<<" show output affect return"<<endl;
            if(ans.find(v[i]) == ans.end()) {
                ans.insert(v[i]);
                q.push(v[i]);
            }
        }

        // return link edge
        if(curr->return_link != NULL && ans.find(curr->return_link) == ans.end()) {
            // cout<<curr->statement<<" -> "<<curr->return_link->statement<<" show output return link"<<endl;
            ans.insert(curr->return_link);
            q.push(curr->return_link);
        }
    }

    unordered_set<string> temp_line_numbers;

    for(auto it:ans) {
        // cout<<it->line_number<<" : "<<it->statement<<endl;
        temp_line_numbers.insert(it->line_number);
    }

    vector<string> ans_temp;
    for(auto it:temp_line_numbers) {
        ans_temp.push_back(it);
    }
    sort(ans_temp.begin(), ans_temp.end(), cmp);

    for(int j=0;j<ans_temp.size();j++) {
        cout<<ans_temp[j]<<" : "<<code[ans_temp[j]]->statement<<"\n";
    }
}

int can_insert(string line, int number) {
    int i = 0;
    string temp = "";
    while(i < line.length() && line[i] == ' ') {
        i++;
    }
    
    if(i < line.length() && line[i] == '{') {
        string temp = source_code[number-1];
        int j = 0;
        while(j < temp.length() && temp[j] == ' ') {
            j++;
        }
        string temp1 = "";
        while(j < temp.length() && temp[j] <= 122 && temp[j] >= 97) {
            temp1 += temp[j++];
        }

        if(temp1 == "for") {
            for_loop_flag = 1;
        } else if(temp1 == "while") {
            while_loop_flag = 1;
        } else if(temp1 == "if" || temp1 == "else") {
            condition_flag = 1;
        }

        if(temp1 == "for" || temp1 == "while" || temp1 == "if" || temp1 == "else") {
            control_dependence2.push(temp1);
        } else {
            temp1 = "";
            while(j < temp.length() && temp[j] == ' ') {
                j++;
            }
            while(j < temp.length() && temp[j] <= 122 && temp[j] >= 97) {
                temp1 += temp[j++];
            }
            function_starting = 1;
            if(temp1 == "main") {
                control_dependence2.push("main_function");
            } else {
                control_dependence2.push("other_function");
            }
        }
        return 1;
    }

    if(i < line.length() && line[i] == '}') {
        return 4;
    }

    while(i < line.length() && line[i] >= 97 && line[i] <= 122) {
        temp += line[i++];
    }

    if(line[0] == '#' || temp == "using") {
        // if(temp == "if") {
        //     conditional_statements.clear();
        //     conditional_statements.push_back(number);
        // } else if(temp == "else") {
        //     conditional_statements.push_back(number);
        // }
        return 0;
    }

    if(temp == "if") {
        return 6;
    }

    if(temp == "else") {
        return 7;
    }

    if(temp == "while") {
        return 3;
    }

    if(temp == "for") {
        return 5;
    }

    while(i < line.length() && line[i] != '(') {
        i++;
    }

    if(i < line.length()) {
        if(line[line.length()-1] == ';') {
            return 2;
        }
        return 0;
        // control_dependence2.push(file_opening_line);
    }
    // if(temp == "main" && i < line.length() && line[i] == '(') {
    //     string temp_line = "\"" + to_string(number) + "#\"";
    //     file_opening_line = "ofstream result;\nresult.open(\"result.txt\");\nresult<<" + temp_line + ";\n";
    //     return 0;
    // }

    // while(i < line.length()) {
    //     temp = "";
    //     while(i < line.length() && !(line[i] >= 97 && line[i] <= 122)) {
    //         i++;
    //     }
    //     while(i < line.length() && (line[i] >= 97 && line[i] <= 122)) {
    //         temp += line[i++];
    //     }
    //     if(temp == "main" && i < line.length() && line[i] == '(') {
    //         string temp_line = "\"" + to_string(number) + "#\"";
    //         file_opening_line = "ofstream result;\nresult.open(\"result.txt\");\nresult<<" + temp_line + ";\n";
    //         return 0;
    //     }
    // }
    if(control_dependence2.empty()) {
        return 0;
    }
    return 2;
}

int main() {
    keywords_init();
    source_code.push_back("");
    string line;
    ifstream fin; 
    fin.open("source.cpp"); 
    ofstream output;
    output.open("output.cpp");
    output<<"#include<sstream>\n#include<fstream>\n";
    string error_line_number;
    string variable_name;
    cout<<"Enter line number where you find an error(format in case of loop is [line number]_[iteration number starting at 0]) : ";
    cin>>error_line_number;
    cout<<"Enter variable name : ";
    cin>>variable_name;
    cout<<"Source file parsing is in progress...\n";
    int line_count = 0;
    vector<string> temp1;
    string temp2;
    stringstream temp2_line(error_line_number);
    while(getline(temp2_line, temp2, '_')) {
        temp1.push_back(temp2);
    }
    bool error_in_loop = 0, loop_closed = 0;
    if(temp1.size() > 1) {
        error_in_loop = 1;
    }
    int end = stoi(temp1[0]);
    while (fin) { 
        getline(fin, line);
        source_code.push_back(line);
        line_count++;
        if((error_in_loop && loop_closed) || (!error_in_loop && line_count > end)) {
            output<<line+"\n";
            if(fin.eof()) {
                break;
            }
            continue;
        }
        int flag = can_insert(line, line_count);
        if(flag == 2) {   // general case

            string temp_line;
            if(while_loop_flag || for_loop_flag) {
                temp_line = "\"" + to_string(line_count) + "_\"" + "<<to_string(loop_counter)" + "<<\"#\"";
            } else {
                temp_line = "\"" + to_string(line_count) + "#\"";
            }
            output<<"result<<" + temp_line + ";\n"; 
            output<<line+"\n";
        } else if(flag == 1) { // "{" case
            output<<line+"\n";
            // for(int i=0;i<conditional_statements.size();i++) {
            //     string temp_line;
            //     if(while_loop_flag || for_loop_flag) {
            //         break;
            //     } else {
            //         temp_line = "\"" + to_string(conditional_statements[i]) + "#\"";
            //     }
            //     output<<"result<<" + temp_line + ";\n";
            // }
            if(control_dependence2.top() == "main_function") {
                string temp_line;
                temp_line = for_main_file_opening_line + "\"" + to_string(line_count - 1) + "#\";\nresult<<\"" + to_string(line_count) + "#\";\n";
                output<<temp_line;
            } else if(control_dependence2.top() == "other_function") {
                string temp_line;
                temp_line = file_opening_line + "\"" + to_string(line_count - 1) + "#\";\nresult<<\"" + to_string(line_count) + "#\";\n";
                output<<temp_line;
            } else {
                string temp_line;
                if(while_loop_flag || for_loop_flag) {
                    loop_closed = 0;
                    temp_line = "\"" + to_string(line_count) + "_\"" + "<<to_string(loop_counter)" + "<<\"#\"";
                } else {
                    temp_line = "\"" + to_string(line_count) + "#\"";
                }
                output<<"result<<" + temp_line + ";\n"; 
            }
        } else if(flag == 3) { // while loop case
            int j = 0;
            while(j < line.length() && line[j] != '(') {
                j++;
            }
            j++;
            string temp1 = line.substr(0, j);
            string temp2 = line.substr(j);
            string temp_line = temp1;
            temp_line += "result<<\"" + to_string(line_count) + "_\"" + "<<to_string(loop_counter)<<\"#\" && " + temp2 + "\n";
            output<<"int loop_counter = 0;\n";
            output<<temp_line;
        } else if(flag == 4) { // scope closing case
            if(control_dependence2.top() == "if" || control_dependence2.top() == "else") {
                string temp_line;
                if(while_loop_flag || for_loop_flag) {
                    temp_line = "\"" + to_string(line_count) + "_\"" + "<<to_string(loop_counter)" + "<<\"#\"";
                } else {
                    temp_line = "\"" + to_string(line_count) + "#\"";
                }
                output<<"result<<" + temp_line + ";\n"; 
                condition_flag = 0;
            } else if(control_dependence2.top() == "for" || control_dependence2.top() == "while") {
                string temp_line = "\"" + to_string(line_count) + "_\"<<to_string(loop_counter)<<\"#\"";
                output<<"result<<" + temp_line + ";\n";
                output<<"loop_counter = loop_counter + 1;\n";
                if(control_dependence2.top() == "for") {
                    for_loop_flag = 0;
                } else {
                    while_loop_flag = 0;
                }
                loop_closed = 1;
            } else {
                output<<"result<<\"" + to_string(line_count) + "#\";\n";
                output<<"result.close();\n";
                function_starting = 0;
            }
            output<<line+"\n";
            control_dependence2.pop();
        } else if(flag == 5) { // for loop case
            int j = 0;
            while(j < line.length() && line[j] != ';') {
                j++;
            }
            j++;
            string temp1 = line.substr(0, j);
            string temp2 = line.substr(j);
            string temp_line = temp1;
            temp_line += "result<<\"" + to_string(line_count) + "_\"" + "<<to_string(loop_counter)<<\"#\" && " + temp2 + "\n";
            output<<"int loop_counter = 0;\n";
            output<<temp_line;
        } else if(flag == 6) { // if case
            int j = 0;
            while(j < line.length() && line[j] != '(') {
                j++;
            }
            j++;
            string temp1 = line.substr(0, j);
            string temp2 = line.substr(j);
            string temp3 = temp1 + "result<<\"" + to_string(line_count);
            if(for_loop_flag || while_loop_flag) {
                temp3 += "_\"<<to_string(loop_counter)<<\"#\" && ";
            } else {
                temp3 += "#\" && ";
            }
            temp3 += temp2 + "\n";
            output<<temp3;
        } else if(flag == 7) { // else case
            int j = 0;
            while(j < line.length() && line[j] != '(') {
                j++;
            }
            string temp3;
            if(j == line.length()) {
                temp3 = " if(result<<\"" + to_string(line_count);
                if(for_loop_flag || while_loop_flag) {
                    temp3 += "_\"<<to_string(loop_counter)<<\"#\")\n";
                } else {
                    temp3 += "#\")\n";
                }
            } else {
                j++;
                string temp1 = line.substr(0, j);
                string temp2 = line.substr(j);
                temp3 = temp1 + "result<<\"" + to_string(line_count);
                if(for_loop_flag || while_loop_flag) {
                    temp3 += "_\"<<to_string(loop_counter)<<\"#\" && ";
                } else {
                    temp3 += "#\" && ";
                }
                temp3 += temp2 + "\n";
            }
            output<<temp3;
        } else { // #, main, using case
            output<<line+"\n";
        }
        // process(line);
        if(fin.eof()) {
            break;
        }
    } 
    fin.close();
    output.close();

    find_trace();

    show_output(error_line_number, variable_name);
}