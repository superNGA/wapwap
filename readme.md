<p align="center">
  <img src="res/cover_art.png" width="500" height="500" alt="cover_art">
</p>
**wapwap** is a "output text line-by-line to a pdf file" tool.
**wapwap** can output plain text, contents of a file, or output of a shell cmd.

usage : ./wapwap configfile[required] taskfile[required]


## Config File
Config file is used to define the visuals of the generated pdf file. Default visuals will be used in case of an empty config file.
Following are the recognized config variables :

```
LINENUM               [int]               : 0 -> disables line number for code-blocks. !0 enables it.
HEADING_FONTFILE      [font file path]    : relative path to the .ttf file to be used for the headings.
HEADING_FONTSIZE      [float]             : font size for headings.
HEADING_CLR           [float float float] : define color of the heading in vec3 format.
HEADING_DECOR_CLR     [float float float] : define color of the heading decoration in vec3 format.
CODETEXT_FONTFILE     [font file path]    : relative path to the .ttf file to be used for code-block content.
CODE_FONTSIZE         [float]             : font size for code-block content.
CODETEXT_CLR          [float float float] : define color of "non-comment" lines for the code-block. 
CODE_COMMENT_FONTFILE [font file path]    : relative path to the .ttf file to be used for "comments" in code-block.
CODECOMMENT_CLR       [float float float] : define color of "comment" lines for the code-block.
CODELINE_CLR_A        [float float float] : define first color for the code block background.
CODELINE_CLR_B        [float float float] : define second color for the code block background.
TEXT_FONTFILE         [font file path]    : relative path to the .ttf file to be used for normal text.
TEXT_FONTSIZE         [float]             : font size for normal text.
TEXT_CLR              [float float float] : color of normal text.
```


## Task File
Task file should be created to define the contents of the pdf file.
Each line of the task file must begin with a **file_entity** symbol (ex. <c>). The following **file_entities** are recognized.
```
<c> : all children are placed in a code-block.
<h> : all children are concatinated into one string and writen as a heading. Truncated at page-width.
<t> : all children are placed as ordinary text.
<p> : Guarantees the next line will be the first line of its page. Creates new page if necessary.
```

Each file entity can have multiple children. Each child begin with a special symbol representing its source type. (ex. [f]). 
The following child source types are recognized :
```
[t] : treats the following text as simple string. Prints as it is.
[f] : treats the following text as a file path. Prints the contents of the file.
[c] : treats the following text as a shell cmd. Prints the output of the shell cmd.
```

Example task file:
```
<h>[t]My Assignment :)
<c>[f]./my_assignment.c
<t>[c]gcc ./my_assignment.c -o a.out; ./a.out; rm ./a.out;
```
This task file generates a pdf, with heading as "My Assignment" followed by the contents of the file "my_assignment.c" placed in a code-block followed by the output of the programed upon execution.
further more, the last line can be modified to the following to make it look more "terminal-like".
```
<t>[t]> g++ ./practical_001.cpp -o a.out; [c]g++ ./practical_001.cpp -o a.out; ./a.out;
```
This prints the terminal command along with the output.
