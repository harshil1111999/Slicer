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
unordered_set<string> datatypes; // for inbuilt datatypes
unordered_set<string> function_names; // for user defined functions
unordered_map<string, unordered_map<string, string>> variable_to_location_map; // map of line_number -> variable_name -> address_of_variable
// unordered_map<string, string> name_to_number; // in case of 'o', 'p', 'ret', 'arg'
unordered_map<string, string> variable_to_pointer_map; // map of variable to it's reference variable
unordered_map<string, vector<string>> definition;
unordered_map<string, vector<vector<string>>> use;
unordered_map<string, vector<int>> dynamic_slice;
unordered_map<string, int> last_statement;
stack<string> if_else_statement;
unordered_set<string> visited_lines; // for in case of for loop to check if it is first time or not
string file_opening_line = "result.open(\"result.txt\");\n";
string file_opening_line2 = "info.open(\"info.txt\");\n";
bool file_object_added = 0;
vector<string> source_code;
stack<string> control_dependence;
string last_dependence;
unordered_map<string,string> variable_as_parameter;


void keywords_init() {
    keywords["for"] = 1;
    keywords["while"] = 2;
    keywords["if"] = 3;
    keywords["else"] = 4;
    keywords["cin"] = 5;
    keywords["cout"] = 6;
    keywords["return"] = 7;
    keywords["endl"] = 8;

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

void find_slice(string number) {
    vector<string> def_temp = definition[number];
    vector<vector<string>> use_temp = use[number];

    stringstream ss(number);
    string line_number;
    getline(ss, line_number, ',');
    for(int i=0;i<def_temp.size();i++) {
        for(int j=0;j<use_temp[i].size();j++) {
            if(dynamic_slice.find(use_temp[i][j]) != dynamic_slice.end()) {
                vector<int> temp = dynamic_slice[use_temp[i][j]];
                for(int k=0;k<temp.size();k++) {
                    dynamic_slice[def_temp[i]].push_back(temp[k]);
                    // cout<<def_temp[i]<<" -> "<<temp[k]<<endl;
                }
            }
            dynamic_slice[def_temp[i]].push_back(last_statement[use_temp[i][j]]);
            // cout<<def_temp[i]<<" -> "<<last_statement[use_temp[i][j]]<<endl;
        }
        last_statement[def_temp[i]] = stoi(line_number);
    }
}

// find defined and used variables and create node for the statement and push node into code map for defined variable
void process_definition(string line, int currentIndex, string number) {
    stringstream ss(line);
    string temp;

    // find defined variable
    getline(ss, temp, '=');
    int i = 0;
    pair<int,string> p;
    while(i < temp.length()) {
        p = find_token(i, temp);
        i = p.first;
        if(p.second != "" && datatypes.find(p.second) == datatypes.end()) {
            break;
        }
    }
    if(p.second != "") {
        string address;

        if(i-p.second.length()-1 >= 0 && temp[i-p.second.length()-1] == '*') {
            if(variable_as_parameter.find(p.second) != variable_as_parameter.end()) {
                address = variable_to_pointer_map[variable_as_parameter[p.second]];
            } else {
                address = variable_to_pointer_map[variable_to_location_map[number][p.second]];
            }
        } else {
            address = variable_to_location_map[number][p.second];
        }

        definition[number].push_back(address);
        vector<string> temp_v;
        use[number].push_back(temp_v);
    }

    // find used variables
    getline(ss, temp, '=');
    i = 0;
    while(i < temp.length()) {
        pair<int,string> p = find_token(i, temp);
        i = p.first;
        if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57)) {
            if((i-p.second.length()-1 >= 0 && temp[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);

                // in case of pointer
                if(i-p.second.length()-1 >= 0 && temp[i-p.second.length()-1] == '*') {
                    if(variable_as_parameter.find(p.second) != variable_as_parameter.end()) {
                        address = variable_as_parameter[p.second];
                    }
                    use[number][use[number].size()-1].push_back(variable_to_pointer_map[address]);
                }
            } else if(i-p.second.length()-1 >= 0 && temp[i-p.second.length()-1] == '&') {
                string address = variable_to_location_map[number][p.second];
                variable_to_pointer_map[definition[number][0]] = address;
            }
        }
    }

    if(!control_dependence.empty() && function_names.find(control_dependence.top()) == function_names.end()) {
        use[number][use[number].size()-1].push_back(control_dependence.top() + "(" + to_string(control_dependence.size()) + ")");
    }
}

void process_for(string line, int currentIndex, string number) {
    int i = currentIndex;
    string temp_line = line.substr(i+1);
    stringstream ss(temp_line);
    string temp1, temp2, temp3;
    getline(ss, temp1, ';');
    getline(ss, temp2, ';');
    getline(ss, temp3, ';');
    stringstream ss1(number);
    string temp_number;
    getline(ss1, temp_number, ',');
    if(visited_lines.find(temp_number) == visited_lines.end()) {
        visited_lines.insert(temp_number);
        process_definition(temp1, 0, number);
    } else {
        process_definition(temp3, 0, number);
    }

    // for defined variables
    definition[number].push_back("p" + temp_number + "(" + to_string(control_dependence.size() + 1) + ")");
    vector<string> temp_v;
    use[number].push_back(temp_v);

    // for used variables
    i = 0;
    while(i < temp2.length()) {
        pair<int,string> p = find_token(i, temp2);
        i = p.first;
        if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57)) {
            if((i-p.second.length()-1 >= 0 && temp2[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);

                // in case of pointer
                if(i-p.second.length()-1 >= 0 && temp2[i-p.second.length()-1] == '*') {
                    if(variable_as_parameter.find(p.second) != variable_as_parameter.end()) {
                        address = variable_as_parameter[p.second];
                    }
                    use[number][use[number].size()-1].push_back(variable_to_pointer_map[address]);
                }
            } else if(i-p.second.length()-1 >= 0 && temp2[i-p.second.length()-1] == '&') {
                string address = variable_to_location_map[number][p.second];
                variable_to_pointer_map[definition[number][0]] = address;
            }
        }
    }

    if(!control_dependence.empty() && function_names.find(control_dependence.top()) == function_names.end()) {
        use[number][use[number].size()-1].push_back(control_dependence.top() + "(" + to_string(control_dependence.size()) + ")");
    }
}

void process_while(string line, int currentIndex, string number) {

    // for defined variables
    stringstream ss(number);
    string temp;
    getline(ss, temp, ',');
    definition[number].push_back("p" + temp + "(" + to_string(control_dependence.size() + 1) + ")");
    vector<string> temp_v;
    use[number].push_back(temp_v);

    // for used variables
    int i = currentIndex;
    while(i < line.length()) {
        pair<int,string> p = find_token(i, line);
        i = p.first;
        if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57)) {
            if((i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);

                // in case of pointer
                if(i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] == '*') {
                    if(variable_as_parameter.find(p.second) != variable_as_parameter.end()) {
                        address = variable_as_parameter[p.second];
                    }
                    use[number][use[number].size()-1].push_back(variable_to_pointer_map[address]);
                }
            } else if(i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] == '&') {
                string address = variable_to_location_map[number][p.second];
                variable_to_pointer_map[definition[number][0]] = address;
            }
        }
    }

    if(!control_dependence.empty() && function_names.find(control_dependence.top()) == function_names.end()) {
        use[number][use[number].size()-1].push_back(control_dependence.top() + "(" + to_string(control_dependence.size()) + ")");
    }
}

void process_if(string line, int currentIndex, string number) {

    // to check the statement is if or else-if
    pair<int,string> token = find_token(0, line);
    if(token.second == "if") {
        while(!if_else_statement.empty()) {
            if_else_statement.pop();
        }
    }

    // for defined variables
    stringstream ss(number);
    string temp;
    getline(ss, temp, ',');
    definition[number].push_back("p" + temp + "(" + to_string(control_dependence.size() + 1) + ")");
    vector<string> temp_v;
    use[number].push_back(temp_v);

    // for used variables
    int i = currentIndex;
    while(i < line.length()) {
        pair<int,string> p = find_token(i, line);
        i = p.first;
        if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57) && keywords.find(p.second) == keywords.end()) {
            if((i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);

                // in case of pointer
                if(i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] == '*') {
                    if(variable_as_parameter.find(p.second) != variable_as_parameter.end()) {
                        address = variable_as_parameter[p.second];
                    }
                    use[number][use[number].size()-1].push_back(variable_to_pointer_map[address]);
                }
            } else if(i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] == '&') {
                string address = variable_to_location_map[number][p.second];
                variable_to_pointer_map[definition[number][0]] = address;
            }
        }
    }

    if(!if_else_statement.empty()) {
        use[number][use[number].size()-1].push_back(if_else_statement.top());
    } else if(!control_dependence.empty() && function_names.find(control_dependence.top()) == function_names.end()) {
        use[number][use[number].size()-1].push_back(control_dependence.top() + "(" + to_string(control_dependence.size()) + ")");
    }
    if_else_statement.push("p" + temp + "(" + to_string(control_dependence.size() + 1) + ")");
}

void process_else(string line, int currentIndex, string number) {
    int i = currentIndex;
    while(i < line.length()) {
        pair<int,string> token = find_token(i, line);
        
        i = token.first;
        string temp = token.second;

        if(temp == "if") {
            process_if(line, i, number);
            use[number][use[number].size()-1].push_back(last_dependence + "(" + to_string(control_dependence.size()+1) + ")");
            return;
        }
    }
    if(!if_else_statement.empty()) {
        stringstream ss(number);
        string temp;
        getline(ss, temp, ',');
        definition[number].push_back("p" + temp + "(" + to_string(control_dependence.size() + 1) + ")");
        vector<string> v;
        use[number].push_back(v);
        use[number][use[number].size()-1].push_back(if_else_statement.top());
    } else {
        cout<<"error in if_else_statement\n";
    }
}

void process_cin(string line, int currentIndex, string number) {
    int i = currentIndex;
    while(i < line.length()) {
        pair<int,string> token = find_token(i, line);
        i = token.first;
        string temp = token.second;
        if(temp == "") {
            continue;
        }
        string address = variable_to_location_map[number][temp];
        definition[number].push_back(address);
        vector<string> temp_v;
        use[number].push_back(temp_v);
    }

    if(!control_dependence.empty() && function_names.find(control_dependence.top()) == function_names.end()) {
        for(int i=0;i<definition[number].size();i++) {
            use[number][i].push_back(control_dependence.top() + "(" + to_string(control_dependence.size()) + ")");
        }
    }
}

//not cosidered case of "variable_name"
void process_cout(string line, int currentIndex, string number) {
    // for defined variables
    stringstream ss(number);
    string temp;
    getline(ss, temp, ',');
    definition[number].push_back("o" + temp);
    vector<string> temp_v;
    use[number].push_back(temp_v);

    // for used variables
    int i = currentIndex;
    while(i < line.length()) {
        pair<int,string> p = find_token(i, line);
        i = p.first;
        if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57) && keywords.find(p.second) == keywords.end()) {
            if((i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);

                // in case of pointer
                if(i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] == '*') {
                    if(variable_as_parameter.find(p.second) != variable_as_parameter.end()) {
                        address = variable_as_parameter[p.second];
                    }
                    use[number][use[number].size()-1].push_back(variable_to_pointer_map[address]);
                }
            } else if(i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] == '&') {
                string address = variable_to_location_map[number][p.second];
                variable_to_pointer_map[definition[number][0]] = address;
            }
        }
    }

    if(!control_dependence.empty() && function_names.find(control_dependence.top()) == function_names.end()) {
        for(int i=0;i<definition[number].size();i++) {
            use[number][i].push_back(control_dependence.top() + "(" + to_string(control_dependence.size()) + ")");
        }
    }
}

void process_function_call(string line, string number, int currentIndex, bool return_value_expects) {

    int j = currentIndex - 1;
    while(j >= 0 && line[j] == ' ') {
        j--;
    }

    string function_name = "";
    while(j >= 0 && ((line[j] >= 65 && line[j] <= 90) || (line[j] >= 97 && line[j] <= 122) || line[j] == '_')) {
        function_name = line[j] + function_name;
        j--;
    }

    stringstream ss(number);
    string temp;
    getline(ss, temp, ',');
    if(visited_lines.find(temp) == visited_lines.end()) {
        if(return_value_expects) {
            visited_lines.insert(temp);
        }
        int i = currentIndex;
        int count = 0;
        while(i < line.length()) {
            pair<int,string> p = find_token(i, line);
            i = p.first;
            if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57) && keywords.find(p.second) == keywords.end()) {  
                count++;
                definition[number].push_back("arg(" + function_name + "," + to_string(count) + ")");
                vector<string> temp_v;
                use[number].push_back(temp_v);
                if((i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                    string address = variable_to_location_map[number][p.second];
                    use[number][use[number].size()-1].push_back(address);

                    // in case of pointer
                    if(i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] == '*') {
                        if(variable_as_parameter.find(p.second) != variable_as_parameter.end()) {
                            address = variable_as_parameter[p.second];
                        }
                        use[number][use[number].size()-1].push_back(variable_to_pointer_map[address]);
                    }
                } else if(i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] == '&') {
                    string address = variable_to_location_map[number][p.second];
                    variable_to_pointer_map[definition[number][definition[number].size()-1]] = address;
                }
                // use[number][use[number].size()-1].push_back(variable_to_location_map[number][p.second]);
            }
        }
    } else {
        visited_lines.erase(temp);

        stringstream temp_line(line);
        string temp1, temp2;
        getline(temp_line, temp1, '=');
        getline(temp_line, temp2, '=');

        // for left side of '='
        int i = 0;
        while(i < temp1.length()) {
            pair<int,string> p = find_token(i, temp1);
            i = p.first;
            if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57)
             && keywords.find(p.second) == keywords.end()) {  
                string address = variable_to_location_map[number][p.second];
                definition[number].push_back(address);
                vector<string> temp_v;
                use[number].push_back(temp_v);
            }
        }

        // for right side of '='
        i = 0;
        use[number][use[number].size()-1].push_back("ret(" + function_name + ")");
        while(i < temp2.length()) {
            pair<int,string> p = find_token(i, temp2);
            i = p.first;
            if(p.second == function_name) {
                while(i < temp2.length() && temp2[i] != ')') {
                    i++;
                }
            } else if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57)
             && keywords.find(p.second) == keywords.end()) {  
                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);
            }
        }
    }

    if(!control_dependence.empty() && function_names.find(control_dependence.top()) == function_names.end()) {
        for(int i=0;i<definition[number].size();i++) {
            use[number][i].push_back(control_dependence.top() + "(" + to_string(control_dependence.size()) + ")");
        }
    }
}

void process_function_definition(string line, string number, int currentIndex) {

    int j = currentIndex - 1;
    while(j >= 0 && line[j] == ' ') {
        j--;
    }

    string function_name = "";
    while(j >= 0 && ((line[j] >= 65 && line[j] <= 90) || (line[j] >= 97 && line[j] <= 122) || (line[j] >= 48 && line[j] <= 57) || line[j] == '_')) {
        function_name = line[j] + function_name;
        j--;
    }

    function_names.insert(function_name);

    int i = currentIndex;
    int count = 0;
    while(i < line.length()) {
        pair<int,string> p = find_token(i, line);
        i = p.first;
        if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57) 
        && keywords.find(p.second) == keywords.end()) {
            count++;
            string address;

            if(i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] == '*') {
                address = variable_to_pointer_map["arg(" + function_name + "," + to_string(count) + ")"];
                variable_as_parameter[p.second] = "arg(" + function_name + "," + to_string(count) + ")";
            } else {
                address = variable_to_location_map[number][p.second];
            }

            definition[number].push_back(address);
            vector<string> temp_v;
            use[number].push_back(temp_v);
            use[number][use[number].size()-1].push_back("arg(" + function_name + "," + to_string(count) + ")");
        }
    }
}

void process_return(string line, int currentIndex, string number) {

    string function_name = control_dependence.top();

    definition[number].push_back("ret(" + function_name + ")");
    vector<string> temp_v;
    use[number].push_back(temp_v);

    int i = currentIndex;
    while(i < line.length()) {
        pair<int,string> p = find_token(i, line);
        i = p.first;
        if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57) 
        && keywords.find(p.second) == keywords.end()) {
            string address = variable_to_location_map[number][p.second];
            use[number][use[number].size()-1].push_back(address);
        }
    }
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
            stringstream ss(number);
            string temp1, temp2;
            getline(ss, temp1, ',');
            getline(ss, temp2, ',');
            string earlier_line = source_code[stoi(temp1) - 1];
            pair<int,string> p = find_token(0, earlier_line);
            if(keywords.find(p.second) != keywords.end()) {
                string temp = "p" + to_string(stoi(temp1) - 1);
                control_dependence.push(temp);
                // cout<<temp<<" pushed\n";
            } else {
                int j = 0;
                while(j < earlier_line.length() && earlier_line[j] != '(') {
                    j++;
                }
                j--;
                while(j >= 0 && earlier_line[j] == ' ') {
                    j--;
                }
                string fun_name = "";
                while(j >= 0 && ((earlier_line[j] >= 65 && earlier_line[j] <= 90) || (earlier_line[j] >= 97 && earlier_line[j] <= 122) 
                || (earlier_line[j] >= 48 && earlier_line[j] <= 57) || earlier_line[j] == '_')) {
                    fun_name = earlier_line[j] + fun_name;
                    j--;
                }
                control_dependence.push(fun_name);
                // cout<<fun_name<<" pushed\n";
            }
            return;
        }
        if(temp1 == "}") {
            if(!control_dependence.empty()) {
                last_dependence = control_dependence.top();
                // cout<<control_dependence.top()<<" poped\n";
                control_dependence.pop();
            }
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
            } else if(function_names.find(temp1) != function_names.end()) {
                process_function_call(line, number, j, 0);
            } else if(j < line.length()) {
                process_function_definition(line, number, j);
            } 
        } else {
            process_definition(line,0,number);
        }
    }
}

void find_variable_locations() {
    ifstream fin;
    fin.open("info.txt");
    string temp, temp1;
    getline(fin, temp);
    stringstream temp_line(temp);
    while(getline(temp_line, temp1, '#')) {
        stringstream ss(temp1);
        string temp2, temp3, temp4, temp5;
        getline(ss, temp2, ':');
        getline(ss, temp3, ':');
        stringstream ss2(temp3);
        getline(ss2, temp4, ',');
        getline(ss2, temp5, ',');
        variable_to_location_map[temp2][temp4] = temp5;
    }
}

void find_trace(int slicing_line_number, vector<string> variable_names) {
    ifstream fin;
    fin.open("result.txt");
    string temp;
    getline(fin, temp);
    string temp1;
    stringstream temp_line(temp);
    while(getline(temp_line, temp1, '#')) {
        string temp2, temp3;
        stringstream temp2_line(temp1);
        getline(temp2_line, temp2, ',');
        process(source_code[stoi(temp2)], temp1);
        find_slice(temp1);

        if(stoi(temp2) == slicing_line_number) {
            break;
        }
    }
    
    // show output
    set<int> st;
    for(int i=0;i<variable_names.size();i++) {
        string name = variable_to_location_map[temp1][variable_names[i]];
        if(dynamic_slice.find(name) == dynamic_slice.end()) {
            vector<vector<string>> temp_v = use[temp1];
            for(int i=0;i<temp_v.size();i++) {
                for(int j=0;j<temp_v[i].size();j++) {
                    vector<int> temp_v2 = dynamic_slice[temp_v[i][j]];
                    for(int k=0;k<temp_v2.size();k++) {
                        st.insert(temp_v2[k]);
                    }
                    st.insert(last_statement[temp_v[i][j]]);
                }
            }

            stringstream ss(temp1);
            string temp;
            getline(ss, temp, ',');
            st.insert(stoi(temp));
            break;
        }
        vector<int> v = dynamic_slice[name];
        for(int i=0;i<v.size();i++) {
            st.insert(v[i]);
        }
        st.insert(last_statement[name]);
    }

    for(auto it:st) {
        if(it != 0) {
            cout<<it<<" : "<<source_code[it]<<endl;
        }
    }
}

pair<int,int> can_insert(string line, int number) {

    int i = 0;
    while(i < line.length() && line[i] != '(') {
        i++;
    } 
    if(i < line.length()) {
        pair<int,string> p = find_token(0, line);
        if(keywords.find(p.second) == keywords.end()) {

            // add function name into set
            i--;
            while(i >= 0 && line[i] == ' ') {
                i--;
            }
            string name = "";
            while(i >= 0 && ((line[i] >= 65 && line[i] <= 90) || (line[i] >= 48 && line[i] <= 57) || (line[i] >= 97 && line[i] <= 122) || line[i] == '_')) {
                name = line[i] + name;
                i--;
            }

            function_names.insert(name);

            if(datatypes.find(p.second) == datatypes.end()) {
                return {7,0};
            } else {
                i = p.first;
                while(i < line.length() && line[i] != '=') {
                    i++;
                }
                if(i < line.length()) {
                    return {7,0};
                }
            }
            return {4,0};
        }
    }

    if(control_dependence.empty()) {
        return {0, 0};
    }

    pair<int,string> p = find_token(0, line);
    if(p.second == "if" || p.second == "while") {
        return {1, p.first + 1};
    }
    if(p.second == "else") {
        p = find_token(p.first, line);
        if(p.second == "if") {
            return {1, p.first + 1};
        } else {
            return {1, line.length()};
        }
    }
    if(p.second == "cin" || p.second == "cout") {
        return {5,0};
    }
    if(p.second == "for") {
        stringstream ss(line);
        string temp;
        getline(ss, temp, ';');
        return {1, temp.length() + 1};
    }
    if(p.second == "return") {
        return {6,0};
    }
    i = 0;
    while(i < line.length() && (line[i] != '{' && line[i] != '}')) {
        i++;
    }
    if(i < line.length()) {
        if(line[i] == '{') {
            return {2,0};
        }
        if(line[i] == '}') {
            return {3,0};
        }
    }

    i = 0;
    while(i < line.length() && line[i] != '=') {
        i++;
    }
    if(i < line.length()) {
        return {5,0};
    }
    return {0,0};
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
    int size;
    cout<<"Enter line number where you wnat to find slice : ";
    cin>>error_line_number;
    cout<<"Enter number of variables : ";
    cin>>size;
    vector<string> variable_names(size);
    for(int i=0;i<size;i++) {
        cout<<"Enter variable name : ";
        cin>>variable_names[i];
    }
    cout<<"Source file parsing is in progress...\n";
    int line_count = 0;
    while (!fin.eof()) { 
        getline(fin, line);
        source_code.push_back(line);
        line_count++;
        pair<int,int> flag = can_insert(line, line_count);
        if(flag.first == 0) {
            output<<line + "\n";
        } else if(flag.first == 1) { // if, else, for, while
            int len = flag.second;
            if(len == line.length()) {
                output<<line + " if(result<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count+1)" + "<<\"#\")\n";
            } else {
                string temp1 = line.substr(0, len);
                string temp2 = line.substr(len);
                output<<temp1 + "result<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count+1)" + "<<\"#\" && \n";

                // lines for to write into info file
                string temp_line = source_code[line_count];
                int i = 0;
                while(i < temp_line.length() && temp_line[i] != '(') {
                    i++;
                }
                while(i < temp_line.length()) {
                    pair<int,string> p = find_token(i, temp_line);
                    i = p.first;
                    if(p.second != "" && !(p.second[0] >= 48 && p.second[0] <= 57) && datatypes.find(p.second) == datatypes.end()
                    && keywords.find(p.second) == keywords.end() && function_names.find(p.second) == function_names.end()) {
                        output<<"info<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count+1)" + "<<\":\"<<\"" + p.second + "\"<<\",\"<<" + "&" + p.second + "<<\"#\" && \n";
                    }
                }

                output<<temp2 + "\n";
            }
        } else if(flag.first == 2) { // '{'
            output<<line + "\n";
            string earlier_line = source_code[line_count-1];
            pair<int,string> p;
            int i = 0;
            while(i < earlier_line.length()) {
                p = find_token(i, earlier_line);
                i = p.first;
                if(p.second != "" && datatypes.find(p.second) == datatypes.end()) {
                    break;
                }
            }
            
            if(keywords.find(p.second) == keywords.end()) {
                if(p.second == "main") {
                    output<<file_opening_line;
                    output<<file_opening_line2;
                    output<<"count = count + 1;\n";
                }
                output<<"result<<\"" + to_string(line_count-1) + "\"<<\",\"" + "<<to_string(count)" + "<<\"#\"" + ";\n";
            } else {
                output<<"count = count + 1;\n";
                control_dependence.push(p.second);
            }
            // cout<<control_dependence.top()<<" pushed : "<<control_dependence.size()<<endl;
            output<<"count = count + 1;\n";
            output<<"result<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count)" + "<<\"#\"" + ";\n";
            
            // lines for to write into info file
            string temp_line = source_code[line_count - 1];
            p = find_token(0, temp_line);
            if(keywords.find(p.second) == keywords.end()) {
                i = 0;
                while(i < temp_line.length() && temp_line[i] != '(') {
                    i++;
                }
                while(i < temp_line.length()) {
                    pair<int,string> p = find_token(i, temp_line);
                    i = p.first;
                    if(p.second != "" && !(p.second[0] >= 48 && p.second[0] <= 57) && datatypes.find(p.second) == datatypes.end()
                    && keywords.find(p.second) == keywords.end() && function_names.find(p.second) == function_names.end()) {
                        output<<"info<<\"" + to_string(line_count-1) + "\"<<\",\"" + "<<to_string(count-1)" + "<<\":\"<<\"" + p.second + "\"<<\",\"<<" + "&" + p.second + "<<\"#\";\n";
                    }
                }
            }
        } else if(flag.first == 3) { // '}'
            output<<"count = count + 1;\n";
            output<<"result<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count)" + "<<\"#\"" + ";\n";
            output<<line + "\n";
            // cout<<control_dependence.top()<<" poped : "<<control_dependence.size()<<endl;
            control_dependence.pop();
        } else if(flag.first == 4) { // function definition
            if(!file_object_added) {
                file_object_added = 1;
                output<<"ofstream result;\nofstream info;\nint count = 0;\n";
            }
            pair<int,string> p;
            int i = 0;
            while(i < line.length()) {
                p = find_token(i, line);
                i = p.first;
                if(datatypes.find(p.second) == datatypes.end()) {
                    break;
                }
            }
            control_dependence.push(p.second);
            // cout<<control_dependence.top()<<" pushed : "<<control_dependence.size()<<endl;
            output<<line + "\n";
        } else if(flag.first == 5) { // definition
            output<<line + "\n";
            output<<"count = count + 1;\n";
            output<<"result<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count)" + "<<\"#\"" + ";\n";
            
            // lines for to write into info file
            string temp_line = source_code[line_count];
            int i = 0;
            while(i < temp_line.length()) {
                pair<int,string> p = find_token(i, temp_line);
                i = p.first;
                if(p.second != "" && !(p.second[0] >= 48 && p.second[0] <= 57) && datatypes.find(p.second) == datatypes.end()
                && keywords.find(p.second) == keywords.end() && function_names.find(p.second) == function_names.end()) {
                    output<<"info<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count)" + "<<\":\"<<\"" + p.second + "\"<<\",\"<<" + "&" + p.second + "<<\"#\";\n";
                }
            }
        } else if(flag.first == 6) { // return
            output<<"count = count + 1;\n";
            output<<"result<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count)" + "<<\"#\"" + ";\n";

            // lines for to write into info file
            string temp_line = source_code[line_count];
            int i = 0;
            while(i < temp_line.length()) {
                pair<int,string> p = find_token(i, temp_line);
                i = p.first;
                if(p.second != "" && !(p.second[0] >= 48 && p.second[0] <= 57) && datatypes.find(p.second) == datatypes.end() 
                    && keywords.find(p.second) == keywords.end() && function_names.find(p.second) == function_names.end()) {
                    output<<"info<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count)" + "<<\":\"<<\"" + p.second + "\"<<\",\"<<" + "&" + p.second + "<<\"#\";\n";
                }
            }

            output<<"count = count + 1;\n";
            output<<"result<<\"" + to_string(line_count+1) + "\"<<\",\"" + "<<to_string(count)" + "<<\"#\"" + ";\n";
            output<<"count = count - 1;\n";
            output<<line + "\n";
            control_dependence.pop();
        } else if(flag.first == 7) { // function call
            output<<"count = count + 1;\n";
            output<<"result<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count)" + "<<\"#\"" + ";\n";

            // lines for to write into info file
            string temp_line = source_code[line_count];
            int i = 0;
            while(i < temp_line.length()) {
                pair<int,string> p = find_token(i, temp_line);
                i = p.first;
                if(p.second != "" && !(p.second[0] >= 48 && p.second[0] <= 57) && datatypes.find(p.second) == datatypes.end()
                 && keywords.find(p.second) == keywords.end() && function_names.find(p.second) == function_names.end()) {
                    output<<"info<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count)" + "<<\":\"<<\"" + p.second + "\"<<\",\"<<" + "&" + p.second + "<<\"#\";\n";
                }
            }

            output<<line + "\n";

            // for return value
            i = 0;
            while(i < temp_line.length() && temp_line[i] != '=') {
                i++;
            }
            if(i < temp_line.length()) {
                output<<"result<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count)" + "<<\"#\"" + ";\n";
                i = 0;
                while(i < temp_line.length()) {
                    pair<int,string> p = find_token(i, temp_line);
                    i = p.first;
                    if(p.second != "" && !(p.second[0] >= 48 && p.second[0] <= 57) && datatypes.find(p.second) == datatypes.end()
                    && keywords.find(p.second) == keywords.end() && function_names.find(p.second) == function_names.end()) {
                        output<<"info<<\"" + to_string(line_count) + "\"<<\",\"" + "<<to_string(count)" + "<<\":\"<<\"" + p.second + "\"<<\",\"<<" + "&" + p.second + "<<\"#\";\n";
                    }
                }
            }
        }
    } 
    fin.close();
    output.close();

    // execute file
    string s = "g++ -o output output.cpp";
    const char *command = s.c_str();
    system(command);
    system("output.exe");

    // from the info file get the location of variables
    find_variable_locations();

    // find answer
    find_trace(error_line_number, variable_names);

    // remove created files
    remove("output.cpp");
    remove("output.exe");
    remove("info.txt");
    remove("result.txt");

}
