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
using namespace std;
unordered_map<string, int> keywords; // for inbuilt keywords
unordered_map<string, struct node*> code; // maps line number to node
unordered_map<string, vector<struct node*>> variable_name_to_nodes; // for to find out all definition node of variable
unordered_set<string> datatypes; // for inbuilt datatypes
stack<struct node*> control_dependence;
// struct node* last_control_dependenece;
string file_opening_line = "";
vector<string> source_code;
vector<int> conditional_statements;
bool loop_flag = 0;

struct node {
    vector<struct node*> parent;
    bool mark; // for dynamic dependence graph
    string line_number;
    string statement;
    unordered_set<string> used, defined;

    node(string number, string line) {
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

    datatypes.insert("int");
    datatypes.insert("float");
    datatypes.insert("double");
    datatypes.insert("string");
}

//add case of variable name contains '_', '"', ''';
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

void process_for(string line, int currentIndex, string number) {

}

void process_while(string line, int currentIndex, string number) {
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
    code[number] = temp_node;
}

void process_if(string line, int currentIndex, string number) {
    int i = currentIndex;
    struct node* temp_node = new node(number, line);
    if(!control_dependence.empty()) {
        temp_node->parent.push_back(control_dependence.top());
        control_dependence.pop();
    }
    control_dependence.push(temp_node);
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
        }
    }
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

    struct node* temp_node = new node(number,line);
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
        // if(temp1 == "}") {
        //     cout<<number<<" "<<control_dependence.size()<<" ";
        //     if(!control_dependence.empty()) {
        //         last_control_dependenece = control_dependence.top();
        //         control_dependence.pop();
        //     }
        //     // cout<<control_dependence.size()<<endl;
        // } else {
            process_definition(line,0,number);
        // }
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


        // changed


        string temp2;
        stringstream temp2_line(temp1);
        getline(temp2_line, temp2, '_');
        process(source_code[stoi(temp2)], stoi(temp1));
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
    set<int> ans;
    queue<int> q;
    q.push(line_number);
    while(!q.empty()) {
        int curr = q.front();
        ans.insert(curr);
        q.pop();
        vector<struct node*> v = code[to_string(curr)]->parent;
        for(int i=0;i<v.size();i++) {
            if(v[i]->mark) {
                q.push(v[i]->line_number);
            }
        }
    }
    for(auto it:ans) {
        cout<<code[to_string(it)]->statement<<"\n";
    }
}

int can_insert(string line, int number) {
    int i = 0;
    string temp = "";
    while(i < line.length() && line[i] == ' ') {
        i++;
    }
    
    if(i < line.length() && line[i] == '{') {
        return 1;
    }

    if(loop_flag && i < line.length() && line[i] == '}') {
        return 4;
    }

    while(i < line.length() && line[i] >= 97 && line[i] <= 122) {
        temp += line[i++];
    }

    if(temp == "else" || temp == "if" || line[0] == '#' || temp == "using") {
        if(temp == "if") {
            conditional_statements.clear();
            conditional_statements.push_back(number);
        } else if(temp == "else") {
            conditional_statements.push_back(number);
        }
        return 0;
    }

    if(temp == "while") {
        loop_flag = 1;
        return 3;
    }

    if(temp == "main" && i < line.length() && line[i] == '(') {
        string temp_line = "\"" + to_string(number) + "#\"";
        file_opening_line = "ofstream result;\nresult.open(\"result.txt\");\nresult<<" + temp_line + ";\n";
        return 0;
    }

    while(i < line.length()) {
        temp = "";
        while(i < line.length() && !(line[i] >= 97 && line[i] <= 122)) {
            i++;
        }
        while(i < line.length() && (line[i] >= 97 && line[i] <= 122)) {
            temp += line[i++];
        }
        if(temp == "main" && i < line.length() && line[i] == '(') {
            string temp_line = "\"" + to_string(number) + "#\"";
            file_opening_line = "ofstream result;\nresult.open(\"result.txt\");\nresult<<" + temp_line + ";\n";
            return 0;
        }
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
    int error_line_number;
    string variable_name;
    cout<<"Enter line number where you find an error : ";
    cin>>error_line_number;
    cout<<"Enter variable name : ";
    cin>>variable_name;
    cout<<"Source file parsing is in progress...\n";
    int line_count = 0;
    while (fin) { 
        getline(fin, line);
        source_code.push_back(line);
        line_count++;
        if(line_count > error_line_number) {
            output<<line+"\n";
            if(fin.eof()) {
                break;
            }
            continue;
        }

        int flag = can_insert(line, line_count);
        if(flag == 2) {   // general case

            string temp_line;
            if(loop_flag) {
                temp_line = "\"" + to_string(line_count) + "_\"" + "<<to_string(i)" + "<<\"#\"";
            } else {
                temp_line = "\"" + to_string(line_count) + "#\"";
            }
            output<<"result<<" + temp_line + ";\n"; 
            output<<line+"\n";
        } else if(flag == 1) { // "{" case
            output<<line+"\n";
            if(file_opening_line != "") {   //only add if earlier line is main method's line
                output<<file_opening_line;
                file_opening_line = "";
            }
            for(int i=0;i<conditional_statements.size();i++) {
                string temp_line;
                if(loop_flag) {
                    break;
                } else {
                    temp_line = "\"" + to_string(conditional_statements[i]) + "#\"";
                }
                output<<"result<<" + temp_line + ";\n";
            }
            string temp_line;
            if(loop_flag) {
                temp_line = "\"" + to_string(line_count) + "_\"" + "<<to_string(i)" + "<<\"#\"";
            } else {
                temp_line = "\"" + to_string(line_count) + "#\"";
            }
            output<<"result<<" + temp_line + ";\n"; 
        } else if(flag == 3) { // loop case
            int j = 0;
            while(j < line.length() && line[j] != '(') {
                j++;
            }
            j++;
            string temp1 = line.substr(0, j);
            string temp2 = line.substr(j);
            string temp_line = temp1;
            temp_line += "result<<\"" + to_string(line_count) + "_\"" + "<<to_string(i)<<\"#\" && " + temp2 + "\n";
            output<<temp_line;
        } else if(flag == 4) { // loop closing case
            string temp_line = "\"" + to_string(line_count) + "_\"<<to_string(i-1)<<\"#\"";
            output<<"result<<" + temp_line + ";\n";
            output<<line+"\n";
            loop_flag = 0;
        } else { // if, else, #, main, using case
            output<<line+"\n";
        }
        // cout << line << endl;
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