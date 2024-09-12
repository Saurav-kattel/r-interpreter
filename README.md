## This is a interpreter written in c.
  It uses RDP technique to parse the Ast
  It is statically typed and type are checked ar runtime


### Variables and Types:
  #### This only support's 2 types numbers and string
     - numbers are just c doubles
     - string are strings and no escape sequeneces
  #### Declaration and assignments
    [ name ]:[Type] = data;
    Eg: hi:string = "hello world";

### Operators
    - Assignment only supports = 
    - Logical are &&,|| and ! as usual
    - Relational are <,>,==, <=,>=, !=,etc
    - dot (.) used for string concatenation
### Inbuilt functions
  #### println 
  As it's name suggest it prints statement to stdout
         println("hi", name, 1,....);
         - supports multiple statements seperated by comma
  #### readIn
  Reads from the stdin
        readIn(data type);
        name:string = readIn(string);
        age:number = readIn(number);
### Arrays
   This supports both static and dynamic arrays
   name[]:string ={"Mike", "Ram"};
   marks[2]:number = {1,2};

### Functions
  syntax for the function decleration is
  
  fn getName(name:string) -> string {  
      return name;
  }


### Loops:
  #### For loop
   for(i:number = 0; i < 2; i = i +1){
      do things
  }
  ### while loop
  i:number = 0;
  while(i < 3){  
    i = i +1;
  }
        
### Control Flow 
  if(conditon){
  do things
  }else{
  do things
  }
  
         
    
