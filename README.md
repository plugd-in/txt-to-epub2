# txt-to-epub2
Convert a delimited or un-delimited collection of text files into an EPUB file.

## WARNING Project incomplete. Version 0.3.0

## Build 
1. Run `cmake .` to generate make files.
2. Run `make` to compile `txt-to-epub2`.
3. Run `make install` to install the program OR move the compiled program into a binary path (SEE STEP 4).
4. To move the file into a binary path, execute `mkdir -p ~/.local/bin` and `export PATH="$PATH:~/.local/bin"`.
5. To make the path change persistent, add the updated PATH environment variable to the .bashrc: `echo "export PATH=\"\$PATH:~/.local/bin\"" >> ~/.bashrc`  

#### Dependencies
* libzip
* glib
* uuid

## Usage 
``` 
Usage: txt-to-epub2 [OPTION...] [INPUT FILE...]
txt-to-epub - Transform .txt document(s) to an epub document.

  -d, --delim=delimiter      Delimiter of the input file. Splits input files
                             into subsequent sections. By default, the entire
                             input file is consumed as a section. Note, the
                             delimiter should be the only line contents.
  -k, --keep                 Keep the construction directory.
  -o, --output=FILE          Output to FILE. Defaults to ./<Document
                             Title>.epub
  -t, --title=TITLE          Title of the document. Defaults to "EPUB Title"
  -v, --verbose              Include verbose output..
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
```

### Examples  
#### Scenario 1: Creating a .epub from a single, delimited ("<------>") text file.
```
$ cat test.txt
  Title 1
  Hello, world!
  <------>
  Title 2
  Goodbye, world!
$ text-to-epub2 -d "<------>" -t "Test EPub" test.txt
  Title: Test EPub
  Delimiter: <------>
```  
#### Scenario 2: Creating a .epub from a list of input files.  
```
$ ls
  1.txt
  2.txt
$ text-to-epub2 -t "Test EPub" 1.txt 2.txt 
  Title: Test EPub
  Delimiter: No Delimiter
```  
#### Scenario 3: Mix of delimited files and non-delimited files.
```
$ ls
  1.txt
  2.txt
$ cat 1.txt
  Title 1
  Hello, world!
  <------>
  Title 2
  Goodbye, world!
$ cat 2.txt
  Title 3
  I forgot, my keys. Goodbye again.
$ text-to-epub2 -d "<------>" 1.txt 2.txt
  Title: EPub Title
  Delimiter: <------>
```
#### Scenario 4: Text from Standard Input.
```
$ cat 1.txt
  Title 1
  Hello, world!
  <------>
  Title 2
  Goodbye, world!
$ cat 2.txt
  Title 3
  I forgot, my keys. Goodbye again.
$ cat 1.txt <(echo "<------>") 2.txt | txt-to-epub2 -d "<------>" -t "Title"
  Title: Title
  Delimiter: <------>
```  

## Feature List/Todo
- [x] Parse files.
- [x] Support Delimiters
- [x] Create construction directories. 
- [x] Support multiple input files.
- [x] Generate Table of Contents.
- [ ] Generate metadata.
- [x] Inmplement compression/file creation to output file.
- [x] More useful output: Chapter titles and/or chapter count.
