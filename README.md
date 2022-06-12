# txt-to-epub2
Convert a delimited or un-delimited collection of text files into an EPUB file.

## Build 
1. Run `cmake .` to generate make files.
2. Run `make` to compile `txt-to-epub2`.
3. Run `make install` to install the program OR move the compiled program into a binary path (SEE STEP 4).
4. To move the file into a binary path, execute `mkdir -p ~/.local/bin` and `export PATH="$PATH:~/.local/bin"`.
5. To make the path change persistent, add the updated PATH environment variable to the .bashrc: `echo "export PATH=\"\$PATH:~/.local/bin\"" >> ~/.bashrc`

## Usage 
``` 
Usage: txt-to-epub [OPTION...] [INPUT FILE...]
txt-to-epub - Transform .txt document(s) to an epub document.

  -d, --delim=delimiter      Delimiter of the input file. Splits input files
                             into subsequent sections. By default, the entire
                             input file is consumed as a section. Note, the
                             delimiter should be the only line contents.
  -k, --keep                 Keep the construction directory.
  -o, --output=FILE          Output to FILE instead of Standard Output.
  -t, --title=TITLE          Title of the document.
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
```
