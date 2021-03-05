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
unordered_map<string,pair<string,string>> function_definition; // for user defined functions (function_name -> {function_start, function_end})
unordered_map<string,string> function_call; // for to keep track of function calling line (line -> function_name)
unordered_map<string, unordered_map<string, string>> variable_to_location_map; // map of line_number -> variable_name -> address_of_variable
unordered_map<string, string> variable_to_pointer_map; // map of variable to it's reference variable
unordered_map<string, vector<string>> definition;
unordered_map<string, vector<vector<string>>> use;
unordered_map<string, unordered_set<int>> dynamic_slice;
unordered_map<string, int> last_statement;
stack<string> if_else_statement;
unordered_set<string> visited_lines; // for in case of for loop and function call to check if it is first time or not
vector<string> source_code;
stack<string> control_dependence;
string last_dependence;
unordered_map<string,string> variable_as_parameter;
unordered_map<string,stack<pair<string,string>>> existing_variables; // for to keep track of scope of variables so that after completing it's scope those variables are removed
unordered_map<string, string> control_dependence_element_to_number;
unordered_map<string, vector<int>> marking_lines; // for dependent lines, mark lines which flags dependent lines to include
unordered_map<string, vector<int>> dependent_lines; // lines like sections and for which haven't processed but should include if marked lines are added
unordered_map<string, int> static_variables; // for to keep track of static variables for threadprivate construct
int address_count = 0;
int clone_bracket_count = 0, section_bracket_count = 0, parallel_bracket_count = 0;
bool do_clone = 0, clone_in_progress = 0, in_section = 0, in_parallel = 0;
pair<int,int> clone_scope = {0,0};
pair<int,int> parallel_scope = {0,0};
ofstream output;
int total_number_of_lines_of_header_files = 0;
void process(string line, string number);
void keywords_init() {
    keywords["for"] = 1;
    keywords["while"] = 2;
    keywords["if"] = 3;
    keywords["else"] = 4;
    keywords["cin"] = 5;
    keywords["cout"] = 6;
    keywords["return"] = 7;
    keywords["endl"] = 8;
    keywords["pragma"] = 9;

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
                unordered_set<int> temp = dynamic_slice[use_temp[i][j]];
                for(auto it:temp) {
                    dynamic_slice[def_temp[i]].insert(it);
                    // cout<<number<<" : "<<def_temp[i]<<" -> "<<it<<endl;
                }
            }
            dynamic_slice[def_temp[i]].insert(last_statement[use_temp[i][j]]);
            // cout<<number<<" : "<<def_temp[i]<<" => "<<last_statement[use_temp[i][j]]<<endl;
        }
        last_statement[def_temp[i]] = stoi(line_number);
    }
}

bool find_slices(int slicing_line_number, string start, string end) {
    bool flag = 0, flag2 = 0, flag3 = 0, barrier_encountered = 0;
    stringstream ss1(start);
    stringstream ss2(end);
    getline(ss1, start, ',');
    getline(ss2, end, ',');
    int low = stoi(start);
    int high = stoi(end);
    for(int i=low;i<=high;i++) {
        string number = to_string(i) + "," + to_string(i);
        
        // to skip lines related to section in which we are slicing
        int j = 0;
        string line = source_code[i];
        if(flag) {
            while(j < line.length() && line[j] == ' ') {
                j++;
            }
        }

        if(!flag || do_clone || (flag && j < line.length() && (line[j] == '{' || line[j] == '}'))) {
            // cout<<number<<" : "<<source_code[i]<<endl;
            process(source_code[i], number);
            if(function_call.find(number) != function_call.end()) {
                find_slice(number);
                if(function_definition.find(function_call[number]) != function_definition.end() &&
                 find_slices(slicing_line_number, function_definition[function_call[number]].first, function_definition[function_call[number]].second)) {
                    return 1;
                }
                process(source_code[i], number);
            }
            // cout<<number<<" : "<<source_code[i]<<endl;
            find_slice(number);
            if(flag && section_bracket_count == 1) {
                flag = 0;
            }
        }

        if(flag2) {
            // to check if barrier encountered when slicing line is in the parallel region
            pair<int,string> p = find_token(0, line);
            p = find_token(p.first, line);
            p = find_token(p.first, line);
            if(p.second == "barrier") {
                barrier_encountered = 1;
            }
        }

        if(i == slicing_line_number) {
            if(in_section || do_clone) {
                flag = 1;
                flag2 = 1;
            } else {
                return 1;
            }
        } else if(flag2 && in_parallel && (parallel_bracket_count == 0 || barrier_encountered)) {
            return 1;
        }

        if(in_parallel && parallel_bracket_count == 0) {
            string line = source_code[i];
            pair<int,string> p = find_token(0, line);
            p = find_token(p.first, line);
            p = find_token(p.first, line);
            if(p.second != "parallel") {
                in_parallel = 0;
                do_clone = 0;
                in_section = 0;
                clone_in_progress = 0;
            }
        }
        // if(clone_in_progress) {
        //     flag3 = 1;
        // }

        // if(flag && in_section) {
        //     flag2 = 1;
        // }
    }
    return 0;
}

void clone_section(int slicing_line_number) {
    for(int i=clone_scope.first;i<=clone_scope.second;i++) {
        string number = to_string(i) + "," + to_string(i);
        process(source_code[i], number);
        if(function_call.find(number) != function_call.end()) {
            find_slice(number);
            find_slices(slicing_line_number, function_definition[function_call[number]].first, function_definition[function_call[number]].second);
            process(source_code[i], number);
        }
        find_slice(number);
    }
}

void repeat_slicing() { // for parallel region so that lines which are defined 
    int low = parallel_scope.first;
    int high = parallel_scope.second;
    for(int i=low;i<=high;i++) {
        string number = to_string(i) + "," + to_string(i);
        find_slice(number);
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
    bool new_variable_flag = 0;
    while(i < temp.length()) {
        p = find_token(i, temp);
        i = p.first;
        if(datatypes.find(p.second) != datatypes.end()) {
            new_variable_flag = 1;
            address_count++;
        }
        if(p.second != "" && datatypes.find(p.second) == datatypes.end()) {
            break;
        }
    }
    if(p.second != "") {
        string address;
        // for to assign unique address to variable
        if(new_variable_flag) {
            variable_to_location_map[number][p.second] = to_string(address_count);
            existing_variables[p.second].push({to_string(address_count),number});
        } else {
            // assign address to variable
            if(existing_variables.find(p.second) != existing_variables.end()) {
                variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
            } else {
                cout<<number<<" : "<<p.second<<" not found in definition\n";
            }
        }

        int j = int(i - p.second.length() - 1);
        while (j >= 0 && temp[j] == ' ') {
            j--;
        }

        if(j >= 0 && temp[j] == '*') {
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
            // assign address to variable
            if(existing_variables.find(p.second) != existing_variables.end()) {
                variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
            } else {
                cout<<number<<" : "<<p.second<<" not found in definition\n";
            }

            if((i-p.second.length()-1 >= 0 && temp[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);

                // in case of pointer
                int j = int(i - p.second.length() - 1);
                while (j >= 0 && temp[j] == ' ') {
                    j--;
                }
                if(j >= 0 && temp[j] == '*') {
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

    if(!control_dependence.empty() && function_definition.find(control_dependence.top()) == function_definition.end()) {
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

    // for to add in marking line
    string earlier_line = source_code[stoi(temp_number) - 1];
    pair<int, string> dependence_token = find_token(0, earlier_line);
    dependence_token = find_token(dependence_token.first, earlier_line);
    dependence_token = find_token(dependence_token.first, earlier_line);
    if (dependence_token.second == "for") {
        marking_lines["for"].push_back(stoi(temp_number));
    }

    // if(visited_lines.find(temp_number) == visited_lines.end()) {
    //     visited_lines.insert(temp_number);
        process_definition(temp1, 0, number);
    // } else {
        process_definition(temp3, 0, number);
    // }

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
            // assign address to variable
            if(existing_variables.find(p.second) != existing_variables.end()) {
                variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
            } else {
                cout<<p.second<<" not found in for loop\n";
            }

            if((i-p.second.length()-1 >= 0 && temp2[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);

                // in case of pointer
                int j = int(i - p.second.length() - 1);
                while (j >= 0 && temp2[j] == ' ') {
                    j--;
                }
                if(j >= 0 && temp2[j] == '*') {
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

    if(!control_dependence.empty() && function_definition.find(control_dependence.top()) == function_definition.end()) {
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
            // assign address to variable
            if(existing_variables.find(p.second) != existing_variables.end()) {
                variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
            } else {
                cout<<p.second<<" not found in while loop\n";
            }

            if((i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);

                // in case of pointer
                int j = int(i - p.second.length() - 1);
                while (j >= 0 && line[j] == ' ') {
                    j--;
                }
                if(j >= 0 && line[j] == '*') {
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

    if(!control_dependence.empty() && function_definition.find(control_dependence.top()) == function_definition.end()) {
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
            // assign address to variable
            if(existing_variables.find(p.second) != existing_variables.end()) {
                variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
            } else {
                cout<<p.second<<" not found in if/else\n";
            }

            if((i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);

                // in case of pointer
                int j = int(i - p.second.length() - 1);
                while (j >= 0 && line[j] == ' ') {
                    j--;
                }
                if(j >= 0 && line[j] == '*') {
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
    } else if(!control_dependence.empty() && function_definition.find(control_dependence.top()) == function_definition.end()) {
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
        // assign address to variable
        if(existing_variables.find(temp) != existing_variables.end()) {
            variable_to_location_map[number][temp] = existing_variables[temp].top().first;
        } else {
            cout<<temp<<" not found in cin\n";
        }

        string address = variable_to_location_map[number][temp];
        definition[number].push_back(address);
        vector<string> temp_v;
        use[number].push_back(temp_v);
    }

    if(!control_dependence.empty() && function_definition.find(control_dependence.top()) == function_definition.end()) {
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
            // assign address to variable
            if(existing_variables.find(p.second) != existing_variables.end()) {
                variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
            } else {
                cout<<p.second<<" not found in cout\n";
            }

            if((i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);

                // in case of pointer
                int j = int(i - p.second.length() - 1);
                while (j >= 0 && line[j] == ' ') {
                    j--;
                }
                if(j >= 0 && line[j] == '*') {
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

    if(!control_dependence.empty() && function_definition.find(control_dependence.top()) == function_definition.end()) {
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
                // assign address to variable
                if(existing_variables.find(p.second) != existing_variables.end()) {
                    variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
                } else {
                    cout<<p.second<<" not found in function call\n";
                }

                count++;
                definition[number].push_back("arg(" + function_name + "," + to_string(count) + ")");
                vector<string> temp_v;
                use[number].push_back(temp_v);
                if((i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] != '&') || i-p.second.length()-1 < 0) {
                    string address = variable_to_location_map[number][p.second];
                    use[number][use[number].size()-1].push_back(address);
                    // in case of pointer
                    int j = int(i - p.second.length() - 1);
                    while (j >= 0 && line[j] == ' ') {
                        j--;
                    }
                    if(j >= 0 && line[j] == '*') {
                        if(variable_as_parameter.find(p.second) != variable_as_parameter.end()) {
                            address = variable_as_parameter[p.second];
                        }
                        use[number][use[number].size()-1].push_back(variable_to_pointer_map[address]);
                    }
                } else if(i-p.second.length()-1 >= 0 && line[i-p.second.length()-1] == '&') {
                    string address = variable_to_location_map[number][p.second];
                    variable_to_pointer_map[definition[number][definition[number].size()-1]] = address;
                }
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
                // assign address to variable
                if(existing_variables.find(p.second) != existing_variables.end()) {
                    variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
                } else {
                    cout<<p.second<<" not found in function call\n";
                }

                string address = variable_to_location_map[number][p.second];
                definition[number].push_back(address);
                vector<string> temp_v;
                use[number].push_back(temp_v);
            }
        }
        // for right side of '='
        i = 0;

        if(function_definition.find(function_name) != function_definition.end()) {
            use[number][use[number].size()-1].push_back("ret(" + function_name + ")");
        } else { // in case of library functions
            stringstream ss(line);
            string temp;
            getline(ss, temp, '(');
            getline(ss, temp, '(');
            getline(ss, temp, ')');
            int j = 0;
            while(j < temp.length()) {
                pair<int,string> p = find_token(j, temp);
                j = p.first;
                if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57) && keywords.find(p.second) == keywords.end()) {  
                    // assign address to variable
                    if(existing_variables.find(p.second) != existing_variables.end()) {
                        variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
                    } else {
                        cout<<p.second<<" not found in function call\n";
                    }
                    string address = variable_to_location_map[number][p.second];
                    use[number][use[number].size()-1].push_back(address);
                }
            }
        }

        while(i < temp2.length()) {
            pair<int,string> p = find_token(i, temp2);
            i = p.first;
            if(p.second == function_name) {
                while(i < temp2.length() && temp2[i] != ')') {
                    i++;
                }
            } else if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57)
             && keywords.find(p.second) == keywords.end()) {  
                // assign address to variable
                if(existing_variables.find(p.second) != existing_variables.end()) {
                    variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
                } else {
                    cout<<p.second<<" not found in function call\n";
                }

                string address = variable_to_location_map[number][p.second];
                use[number][use[number].size()-1].push_back(address);
            }
        }
    }

    if(!control_dependence.empty() && function_definition.find(control_dependence.top()) == function_definition.end()) {
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

    int i = currentIndex;
    int count = 0;
    bool new_variable_flag = 0;
    while(i < line.length()) {
        pair<int,string> p = find_token(i, line);
        i = p.first;
        
        // check if it is new variable
        if(datatypes.find(p.second) != datatypes.end()) {
            new_variable_flag = 1;
            address_count++;
        }

        if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57) 
        && keywords.find(p.second) == keywords.end()) {
            // for to assign unique address to variable
            if(new_variable_flag) {
                variable_to_location_map[number][p.second] = to_string(address_count);
                existing_variables[p.second].push({to_string(address_count),number});
                new_variable_flag = 0;
            } else {
                // assign address to variable
                if(existing_variables.find(p.second) != existing_variables.end()) {
                    variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
                } else {
                    cout<<number<<" : "<<p.second<<" not found in process function definition\n";
                }
            }

            count++;
            string address;

            int j = int(i - p.second.length() - 1);
            while (j >= 0 && line[j] == ' ') {
                j--;
            }
            if(j >= 0 && line[j] == '*') {
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
            if(existing_variables.find(p.second) != existing_variables.end()) {
                variable_to_location_map[number][p.second] = existing_variables[p.second].top().first;
            } else {
                cout<<number<<" : "<<p.second<<" not found in return\n";
            }
            string address = variable_to_location_map[number][p.second];
            use[number][use[number].size()-1].push_back(address);
        }
    }
}

void process_pragma(string line, int currentIndex, string number) {
    stringstream ss(number);
    string temp;
    getline(ss, temp, ',');
    definition[number].push_back("p" + temp + "(" + to_string(control_dependence.size() + 1) + ")");
    vector<string> temp_v;
    use[number].push_back(temp_v);

    // for to add in marking line
    pair<int, string> p = find_token(0, line);
    p = find_token(p.first, line);
    p = find_token(p.first, line);
    if(p.second == "parallel" && !clone_in_progress) {
        parallel_scope = {stoi(temp), 0};
        in_parallel = 1;
        do_clone = 1;
        clone_scope = {stoi(temp),0};
    } else if (p.second == "section") { // for to add pragma omp sections line
        marking_lines["section"].push_back(stoi(temp));
    } else if (p.second == "for") { // for to add pragma omp for line
        // clone_scope = {stoi(temp),0};
        dependent_lines["for"].push_back(stoi(temp));
    } else if (p.second == "sections") { // for to add pragma omp sections line
        in_section = 1;
        do_clone = 0;
        clone_bracket_count = 0;
        clone_scope = {0,0};
        dependent_lines["sections"].push_back(stoi(temp));
    } else if(p.second == "barrier") {
        if(do_clone) {
            clone_scope.second = stoi(temp) - 1;
            clone_in_progress = 1;
            clone_section(0);
            clone_in_progress = 0;
            clone_scope = {stoi(temp) + 1,0};
            parallel_scope.second = stoi(temp) - 1;
            repeat_slicing();
            parallel_scope = {stoi(temp) + 1,0};
        }
    }

    // for to keep track of private variables
    int i = p.first;
    while(i < line.length()) {
        p = find_token(i, line);
        i = p.first;
        if(p.second == "private" || p.second == "firstprivate" || p.second == "lastprivate") {
            while(i < line.length()) {
                p = find_token(i, line);
                i = p.first;
                if(p.second != "") {
                    address_count++;
                    definition[number].push_back(to_string(address_count));
                    vector<string> temp1;
                    use[number].push_back(temp1);
                    use[number][use[number].size()-1].push_back(existing_variables[p.second].top().first);
                    existing_variables[p.second].push({to_string(address_count),number});
                }
            }
        }
        if(p.second == "reduction") {
            string next_number = to_string(stoi(temp) + 1) + "," + to_string(stoi(temp) + 1);
            while(i < line.length()) {
                p = find_token(i, line);
                i = p.first;
                if(p.second != "") {
                    address_count++;
                    definition[next_number].push_back(to_string(address_count));
                    vector<string> temp1;
                    use[next_number].push_back(temp1);
                    use[next_number][use[next_number].size()-1].push_back(existing_variables[p.second].top().first);
                    existing_variables[p.second].push({to_string(address_count),next_number});
                }
            }
        }
    }

    if(!control_dependence.empty() && function_definition.find(control_dependence.top()) == function_definition.end()) {
        use[number][use[number].size()-1].push_back(control_dependence.top() + "(" + to_string(control_dependence.size()) + ")");
    }
}

void remove_related_existing_variables(string number) {
    stringstream ss(number);
    string temp;
    getline(ss, temp, ',');
    int low = stoi(control_dependence_element_to_number[control_dependence.top()]);
    int high = stoi(temp);
    for(auto it=existing_variables.begin();it!=existing_variables.end();it++) {
        if(!it->second.empty()) {
            string curr = it->second.top().second;
            stringstream ss1(curr);
            string temp1;
            getline(ss1, temp1, ',');
            int curr_number = stoi(temp1);
            if(curr_number >= low && curr_number <= high) {
                existing_variables[it->first].pop();
            }
        }
    }
}

void process(string line, string number) {
    int i = 0;
    while(i < line.length() && line[i] == ' ') {
        i++;
    }
    if(i < line.length() && line[i] == '/' && i+1 < line.length() && line[i+1] == '/') {
        return;
    }

    i = 0;
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
            case 9:
                process_pragma(line,i,number);
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
            if(in_parallel) {
                parallel_bracket_count++;
            }
            if(do_clone) {
                clone_bracket_count++;
            }
            if(in_section) {
                section_bracket_count++;
            }
            stringstream ss(number);
            string temp1, temp2;
            getline(ss, temp1, ',');
            getline(ss, temp2, ',');
            string earlier_line = source_code[stoi(temp1) - 1];
            pair<int,string> p = find_token(0, earlier_line);
            if(keywords.find(p.second) != keywords.end()) {
                string temp = "p" + to_string(stoi(temp1) - 1);
                control_dependence.push(temp);
                control_dependence_element_to_number[temp] = to_string(stoi(temp1) - 1);
                // cout<<number<<" "<<temp<<" pushed\n";
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
                control_dependence_element_to_number[fun_name] = to_string(stoi(temp1) - 1);
                // cout<<number<<" "<<fun_name<<" pushed\n";
            }
            return;
        }
        if(temp1 == "}") {
            if(in_parallel) {
                parallel_bracket_count--;
            }
            if(do_clone) {
                clone_bracket_count--;
            }
            if(in_section) {
                section_bracket_count--;
            }
            if(!control_dependence.empty()) {
                last_dependence = control_dependence.top();
                // cout<<number<<" "<<control_dependence.top()<<" poped\n";
                remove_related_existing_variables(number);
                control_dependence.pop();
                string temp = "";
                int i = 0;
                while(i < last_dependence.length() && !(last_dependence[i] >= 48 && last_dependence[i] <= 57)) {
                    i++;
                }
                while(i < last_dependence.length() && (last_dependence[i] >= 48 && last_dependence[i] <= 57)) {
                    temp += last_dependence[i++];
                }
                if(temp != "") {
                    temp = source_code[stoi(temp)];
                    pair<int,string> p = find_token(0, temp);
                    p = find_token(p.first, temp);
                    p = find_token(p.first, temp);
                    if(do_clone && p.second == "parallel") {
                        stringstream ss(number);
                        string temp;
                        getline(ss, temp, ',');
                        clone_scope.second = stoi(temp);
                        do_clone = 0;
                        clone_in_progress = 1;
                        clone_section(0);
                        clone_in_progress = 0;
                        clone_scope = {0,0};
                    }
                }
            }
            string temp = "";
            int i = 0;
            while(i < last_dependence.length() && !(last_dependence[i] >= 48 && last_dependence[i] <= 57)) {
                i++;
            }
            while(i < last_dependence.length() && (last_dependence[i] >= 48 && last_dependence[i] <= 57)) {
                temp += last_dependence[i++];
            }
            if(temp != "") {
                temp = source_code[stoi(temp)];
                pair<int,string> p = find_token(0, temp);
                p = find_token(p.first, temp);
                p = find_token(p.first, temp);
                if(in_parallel && p.second == "parallel") {
                    stringstream ss(number);
                    string temp;
                    getline(ss, temp, ',');
                    parallel_scope.second = stoi(temp);
                    repeat_slicing();
                }
            }
            return;
        }

        // to find if it is function call or definition or else
        j = 0;
        while(j < line.length() && line[j] == ' ') {
            j++;
        }
        temp1 = "";
        bool is_equal_sign_exist = 0;
        while(j < line.length() && line[j] != '(') {
            if(line[j] == '=') {
                is_equal_sign_exist = 1;
            }
            if(line[j] != '(') {
                temp1 += line[j];
            }
            j++;
        }
        if(j < line.length()) {
            if(is_equal_sign_exist) {
                process_function_call(line, number, j, 1);
            } else if(function_definition.find(temp1) != function_definition.end()) {
                process_function_call(line, number, j, 0);
            } else if(j < line.length()) {
                process_function_definition(line, number, j);
            } 
        } else {
            if(is_equal_sign_exist) {
                process_definition(line,0,number);
            } else {
                int i = 0;
                pair<int,string> p = find_token(i, line);
                i = p.first;
                if(datatypes.find(p.second) != datatypes.end()) {
                    while(i < line.length()) {
                        p = find_token(i, line);
                        i = p.first;
                        if(p.second != "" && datatypes.find(p.second) == datatypes.end() && !(p.second[0] >= 48 && p.second[0] <= 57)
                            && keywords.find(p.second) == keywords.end()) {
                            address_count++;
                            existing_variables[p.second].push({to_string(address_count),number});
                        }
                    }
                }
            }
        }
    }
}

int is_function_definition_or_call(string line) {
    pair<int,string> p = find_token(0, line);
    if(keywords.find(p.second) != keywords.end()) {
        return 0;
    }
    stringstream ss(line);
    string temp;
    getline(ss, temp, '(');
    if(!getline(ss, temp, '(')) {
        return 0;
    }
    stringstream ss1(line);
    getline(ss1, temp, ';');
    getline(ss1, temp, ';');
    if(temp == "") {
        return 2;
    }
    return 1;
}

string find_function_name(string line) {
    int i = 0;
    while(i < line.length() && line[i] != '(') {
        i++;
    }
    i--;
    while(i >= 0 && line[i] == ' ') {
        i--;
    }
    string name = "";
    while(i >= 0 && ((line[i] >= 97 && line[i] <= 122) || (line[i] >= 65 && line[i] <= 90) || (line[i] >= 48 && line[i] <= 57) || line[i] == '_')) {
        name = line[i--] + name;
    }
    return name;
}

void find_trace() {
    ifstream fin;
    fin.open("output.cpp");
    source_code.push_back("");
    int count = 0;
    string curr_function;
    while(!fin.eof()) {
        string line;
        count++;
        getline(fin, line);
        source_code.push_back(line);
        string number = to_string(count) + "," + to_string(count);
        int check = is_function_definition_or_call(line);
        if(check == 1) {
            curr_function = find_function_name(line);
            function_definition[curr_function] = make_pair(number,"0,0");
        } else if(check == 2) {
            function_call[number] = find_function_name(line);
        } else {
            int i = 0;
            while(i < line.length() && line[i] == ' ') {
                i++;
            }
            if(i < line.length() && line[i] == '}') {
                function_definition[curr_function].second = number;
            }
        }
    }
}

void show_output(int slicing_line_number, vector<string> variable_names) {
    string number = to_string(slicing_line_number) + "," + to_string(slicing_line_number);
    set<int> st;
    for(int i=0;i<variable_names.size();i++) {
        string name = variable_to_location_map[number][variable_names[i]];
        if(dynamic_slice.find(name) == dynamic_slice.end()) {
            vector<vector<string>> temp_v = use[number];
            for(int i=0;i<temp_v.size();i++) {
                for(int j=0;j<temp_v[i].size();j++) {
                    unordered_set<int> temp_v2 = dynamic_slice[temp_v[i][j]];
                    for(auto it:temp_v2) {
                        st.insert(it);
                    }
                    st.insert(last_statement[temp_v[i][j]]);
                }
            }

            stringstream ss(number);
            string temp;
            getline(ss, temp, ',');
            st.insert(stoi(temp));
            break;
        }
        unordered_set<int> v = dynamic_slice[name];
        for(auto it:v) {
            st.insert(it);
        }
        st.insert(last_statement[name]);
    }

    // for sections line
    vector<int> v1 = dependent_lines["sections"];
    vector<int> v2 = marking_lines["section"];
    for (int i = 0; i < v2.size(); i++) {
        if(st.find(v2[i]) != st.end()) {
            int last = 0;
            for (int j = 0; j < v1.size(); j++) {
                if (v1[j] > v2[i]) {
                    break;
                }
                last = v1[j];
            }
            st.insert(last);
        }
    }

    // for 'for' line
    v1 = dependent_lines["for"];
    v2 = marking_lines["for"];
    for (int i = 0; i < v2.size(); i++) {
        if(st.find(v2[i]) != st.end()) { 
            int last = 0;
            for (int j = 0; j < v1.size(); j++) {
                if (v1[j] > v2[i]) {
                    break;
                }
                last = v1[j];
            }
            st.insert(last);
        }
    }

    for(auto it:st) {
        if(it != 0) {
            cout<<it<<" : "<<source_code[it]<<endl;
        }
    }
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
    int slicing_line_number;
    int size;
    cout<<"Enter line number where you wnat to find slice : ";
    cin>>slicing_line_number;
    cout<<"Enter number of variables : ";
    cin>>size;
    vector<string> variable_names(size);
    for(int i=0;i<size;i++) {
        cout<<"Enter variable name : ";
        cin>>variable_names[i];
    }
    cout<<"Source file parsing is in progress...\n";

    // to add lines of source code to output file which is helpful in case of importing functions from header files
    string line;
    ifstream fin;
    fin.open("tSource.cpp");
    output.open("output.cpp");
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

    find_trace();
    // for(auto it=function_definition.begin();it!=function_definition.end();it++) {
    //     cout<<it->first<<" -> "<<it->second.first<<" : "<<it->second.second<<endl;
    // }

    // for(auto it=function_call.begin();it!=function_call.end();it++) {
    //     cout<<it->first<<" -> "<<it->second<<endl;
    // }
    slicing_line_number += total_number_of_lines_of_header_files;

    find_slices(slicing_line_number, function_definition["main"].first, function_definition["main"].second);

    show_output(slicing_line_number, variable_names);
}