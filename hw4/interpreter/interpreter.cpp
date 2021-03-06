#include "interpreter.h"
#include <sstream>
#include <string>

using namespace std;

Interpreter::Interpreter(std::istream& in) {
    this->parse(in);
}

Interpreter::~Interpreter() {
    // loop through each command and deallocate
    for(int i=0;i<(int)commands.size();i++){
        delete commands[i];
    }
}

void Interpreter::parse(std::istream& in) {
    string line;

    while (getline(in, line)) {
        size_t line_number;
        stringstream stream(line);
        stream >> line_number;

        Command* newCommand = 0;
        // get rid of excess spaces
        // update line to start from command
        line = "";
        while(!stream.eof()){
            string temp;
            stream >> temp;
            line = line + temp + " ";
        }

        // after line number check for command
        newCommand = this->parse_command(line);

        // add to map
        commands[line_number] = newCommand;        
    }
}

void Interpreter::write(std::ostream& out) {
    map<size_t, Command*>::iterator it;
	for(it = commands.begin();it != commands.end();++it){
        out << it->first << " " << it->second->format() << endl;
    }
}

string Interpreter::strip_expression(string line){
    string s = "";
    int starti = -1;
    int endi = -1;
    
    // if line is empty
    if(line == ""){
        return "";
    }

    // iterate through letters from front to find first non whitespace
    for(int i=0;i<(int)line.size();i++){
        // non whitespace
        if(line[i] != ' ' && line[i] != '\t'){
            starti = i;
            break;
        }
    }
    // iterate through letters from back to find last non whitespace
    for(int i=(int)line.size()-1;i>=0;i--){
        // non whitespace
        if(line[i] != ' ' && line[i] != '\t'){
            endi = i;
            break;
        }
    }

    // get the stripped string
    for(int i=starti;i<=endi;i++){
        s += line[i];
    }

    return s;
}

// calls the appropriate parse function to instantiate command
// returns pointer to the command object
Command* Interpreter::parse_command(string line){
	stringstream ss(line);
    string s;
    // Command pointer to instantiate new commands
    Command* newCommand = 0;

    // first input should be command (no spaces allowed)
    ss >> s;

    // update line to not have command (start with next term)
    line = "";
    while(!ss.eof()){
        string temp;
        ss >> temp;
        line = line + temp + " ";
    }

    line = strip_expression(line);

    // check which command it is
    if(s == "PRINT"){
        // catch the pointer to newly instantiated command
        newCommand = this->parse_command_print(line);
    }
    else if(s == "LET"){
        newCommand = this->parse_command_let(line);
    }
    else if(s == "GOTO"){
        newCommand = this->parse_command_goto(line);
    }
    else if(s == "IF"){
        newCommand = this->parse_command_if(line);
    }
    else if(s == "GOSUB"){
        newCommand = this->parse_command_gosub(line);
    }
    else if(s == "RETURN"){
        // instantiate return command
        newCommand = new Command_Return(&commands, &gotos);
    }
    else if(s == "END"){
        // instantiate end command
        newCommand = new Command_End(&commands);
    }

    return newCommand;
}

Command* Interpreter::parse_command_print(string line){
    // line should be nexp
    // parse nexp
    NumericExpression* nexp = parse_nexp(line);

    // instantiate command
    Command* newCommand = new Command_Print(nexp);

    return newCommand;
}

// separate variable from nexp
Command* Interpreter::parse_command_let(string line){
    stringstream ss(line);
    string temp;
    string variable;
    string nexpline = "";
    Command* newCommand = 0;
    Variable* var = 0;
    NumericExpression* nexp = 0;
    bool isArray = false;
    int count = 0;



    // get first term
    ss >> variable;

    // 3 cases after >>
        // single variable so next is nexp
        // array variable [ already accounted for
            // add until ]
        // array variable [ not yet accounted for
            // look at temp[0] and add until ]

    // check if [  and ] accounted for
    for(int i=0;i<(int)variable.size();i++){
        if(variable[i] == '['){
            isArray = true;
            count++;
        }
        if(variable[i] == ']'){
            count--;
        }
    }

    ss >> temp;

    // if '[' => array or already array
    if(temp[0] == '[' || (isArray && count != 0)){
        variable += " " + temp;
        isArray = true;
    }

    // if array => add until ]
    if(isArray && count != 0){
        // check for ']' and read until ']' if not there
        while(variable[(int)variable.size()-1] != ']'){
            ss >> temp;
            variable += " " + temp;
        }
    }
    // if not array => nexp
    else{
        nexpline = nexpline + temp + " ";
    }

    // get rest for nexp
    while(!ss.eof()){
        ss >> temp;
        nexpline = nexpline + temp + " ";
    }

    // strip
    variable = strip_expression(variable);
    nexpline = strip_expression(nexpline);

    // parse variable for instantiation
    var = this->check_variable(variable);
    // parse nexp for instantiation
    nexp = this->parse_nexp(nexpline);

    // instantiate command
    newCommand = new Command_Let(var, nexp);

    return newCommand;
}

Command* Interpreter::parse_command_goto(string line){
    size_t jlnum;
    stringstream ss(line);
    // line should be int that represents jlnum
    ss >> jlnum;

    // instantiate jumpline
    JumpLine* newJumpline = new JumpLine(jlnum);
    // instantiate goto command
    Command* newCommand = new Command_Goto(newJumpline, &commands);

    return newCommand;
}

// separate bexp and jline
Command* Interpreter::parse_command_if(string line){
    stringstream ss(line);
    string temp;
    string bexpline = "";
    size_t lnum;
    Command* newCommand = 0;
    BooleanExpression* bexp;
    JumpLine* jlnum;

    // get bexp until THEN
    ss >> temp;
    while(temp != "THEN"){
        bexpline = bexpline + temp + " ";
        ss >> temp;
    }

    // get int for jline
    ss >> lnum;

    // instantiate jumpline
    jlnum = new JumpLine(lnum);

    // strip
    bexpline = strip_expression(bexpline);
    // parse boolean expression
    bexp = this->parse_bexp(bexpline);

    // instantiate command
    newCommand = new Command_If(bexp, jlnum, &commands);

    return newCommand;
}

Command* Interpreter::parse_command_gosub(string line){
    size_t jlnum;
    stringstream ss(line);
    // line should be int that represents jlnum
    ss >> jlnum;

    // instantiate jumpline
    JumpLine* newJumpline = new JumpLine(jlnum);
    // instantiate gosub command
    Command* newCommand = new Command_Gosub(newJumpline, &commands, &gotos);

    return newCommand;
}

// separate nexp vs nexp
BooleanExpression* Interpreter::parse_bexp(string line){

    // line should be stripped

    // 1 => <, 2 => >, 3 => =
    int type = 0;
    stringstream ss(line);
    string lnexpline = "";
    string rnexpline = "";
    NumericExpression* left = 0;
    NumericExpression* right = 0;
    BooleanExpression* bexp = 0;

    int i = 0;

    for(;i<(int)line.size();i++){
        // if not bexp store in lnexpline
        if(line[i] != '<' && line[i] != '>' && line[i] != '='){
            lnexpline = lnexpline + line[i];
        }
        // if bexp set type & break
        else{
            // determine type
            if(line[i] == '<'){type = 1;}
            if(line[i] == '>'){type = 2;}
            if(line[i] == '='){type = 3;}

            break;
        }
    }

    i++;

    // get right side
    for(;i<(int)line.size();i++){
        rnexpline = rnexpline + line[i];
    }

    // strip
    lnexpline = strip_expression(lnexpline);
    rnexpline = strip_expression(rnexpline);

    // parse each side
    left = this->parse_nexp(lnexpline);
    right = this->parse_nexp(rnexpline);

    // instantiate boolean
    if(type == 1){
        bexp = new LessExpression(left, right);
        return bexp;
    }
    if(type == 2){
        bexp = new GreaterExpression(left, right);
        return bexp;
    }
    if(type == 3){
        bexp = new EqualExpression(left, right);
        return bexp;
    }
    // something went wrong
    else{
        return 0;
    }
}

NumericExpression* Interpreter::parse_nexp(string line){

    // line should already start with first term of nexp
    // line should be stripped

    NumericExpression* nexp;

    // either - or digit
    nexp = this->check_number(line);
    if(nexp != 0){
        return nexp;
    }

    // or variable
    nexp = this->check_variable(line);
    if(nexp != 0){
        return nexp;
    }

    // or binary operator
    nexp = this->check_binop(line);
    if(nexp != 0){
        return nexp;
    }

    // return null => something is wrong
    return 0;

}

Constant* Interpreter::check_number(string s){
    Constant* nexp = 0;

    // check if digit
    for(char i='0';i<='9';i++){
        // if digit => instantiate
        if(s[0] == i){
            int x;
            stringstream sstemp(s);
            sstemp >> x;
            nexp = new Constant(x);

            return nexp;
        }
    }
    // check if -
    // if - => instantiate
    if(s[0] == '-'){
        int x;
        stringstream sstemp(s);
        sstemp >> x;
        nexp = new Constant(x);

        return nexp;
    }

    // if not return null
    return 0;
}

Variable* Interpreter::check_variable(string s){
    string name;
    string nexpline = "";
    string temp;
    stringstream ss;
    Variable* variable = 0;
    bool isVariable = false;
    bool isArray = false;
    bool bracketUnseen = true;

    // variable if starts with letter
    for(char i='A';i<='Z';i++){
        if(s[0] == i){
            isVariable = true;
            break;
        }
    }

    // if not variable return null
    if(!isVariable){
        return 0;
    }

    // check if array
    for(int i=0;i<(int)s.size();i++){
        // if [ => array
        if(s[i] == '[' && bracketUnseen){
            // will get rid of leading whitespace
            ss >> name;
            isArray = true;
            bracketUnseen = false;
            // reset eof
            ss.clear();
        }
        // outermost ] must be at the end
        else if(s[i] == ']' && i == (int)s.size()-1){
            break;
        }
        else{
            ss << s[i];
        }
    }

    // ss now contains either variable name or nexp for index

    // single variable
    if(!isArray){
        // names do not contain spaces
        ss >> name;

        // instantiate
        variable = new SingleVariable(name, &variables);
        // add to variables if not found
        if(variables.find(name) == variables.end()){
            // add to variables
            variables[name] = 0;
        }
        return variable;
    }
    // array
    else{
        while(!ss.eof()){
            ss >> temp;
            nexpline = nexpline + temp + " ";
        }
        // strip
        nexpline = strip_expression(nexpline);
        // parse nexpline to create nexp pointer
        NumericExpression* nexp = this->parse_nexp(nexpline);

        // instantiate
        variable = new ArrayVariable(name, &variables, nexp);

        // update name to include index
        stringstream sstemp;
        sstemp << nexp->getValue();
        string stemp;
        sstemp >> stemp;
        name += "[" + stemp + "]";

        // add to variables if not found
        if(variables.find(name) == variables.end()){
            variables[name] = 0;
        }

        return variable;
    }
}

BinaryExpression* Interpreter::check_binop(string s){

    // line should be stripped

    // + => 1, - => 2, * => 3, / => 4
    int type = 2;
    int i = 0;
    BinaryExpression* binop = 0;
    NumericExpression* left = 0;
    NumericExpression* right = 0;
    bool isBinop = false;
    int count = 0;
    stringstream ss(s);
    string lnexp = "";
    string rnexp = "";

    // if '(' => binary operator
    if(s[0] == '('){
        isBinop = true;
    }

    // return null if not binary operator
    if(!isBinop){
        return 0;
    }

    // iterate through string find binop
    // PROBLEM: WHEN NUMBER IS NEGATIVE => THOUGHT OF AS -
    for(;i<(int)s.size();i++){
        if(count==1 && (s[i]=='+' || s[i]=='*' || s[i]=='/')){
            if(s[i]=='+'){type = 1;}
            if(s[i]=='*'){type = 3;}
            if(s[i]=='/'){type = 4;}
            break;
        }
        // keep count of '(' to find outermost
        // decrease when ')'
        // outermost when count == 1
        if(s[i] == '('){
            count++;
        }
        // store everything from after first '(' to outermost binop
        if(count != 1 || s[i] != '('){
            lnexp = lnexp + s[i];
        }
        if(s[i] == ')'){
            count--;
        }
    }

    i++;
    // get right side 
    for(;i<(int)s.size();i++){
        if(s[i] == '('){
                count++;
        }
        if(s[i] == ')'){
            count--;
        }
        if(count != 0){
            rnexp = rnexp + s[i];
        }
    }

    // strip both expressions
    lnexp = strip_expression(lnexp);
    rnexp = strip_expression(rnexp);

    // parse 2 nexp
    left = this->parse_nexp(lnexp);
    right = this->parse_nexp(rnexp);

    // instantiate
    if(type == 1){
        binop = new AdditionExpression(left, right);
        return binop;
    }
    else if(type == 2){
        binop = new SubtractionExpression(left, right);
        return binop;
    }
    else if(type == 3){
        binop = new MultiplicationExpression(left, right);
        return binop;
    }
    else if(type == 4){
        binop = new DivisionExpression(left, right);
        return binop;
    }
    // somethings wrong
    else{
        delete left;
        delete right;
        return 0;
    }
}

void Interpreter::execute() {
    map<size_t, Command*>::iterator it = this->commands.begin();
    while (it != commands.end()) {
        try{
            it->second->execute(it);    
        }
        catch (runtime_error& e) {
            stringstream ss;
            ss << it->first;
            string s;
            ss >> s;
            cout << "Error on Line " + s + ": " + e.what() << endl;
            return;
        }
    }

}