# Python plugin for CLCL clipboard manager
One of the main advantages of CLCL over other other clipboard managers ist the feature to include plugins to extend CLCL's functions. 
But the plugin architecture is based on C, and even for experienced C it is a tedious work to write a new plugin.  
Python on the other hand is an easy-to-learn programming language.  
Most CLCL plugins are text-based: 
A text selection within an editor window is copied to the clipboard, the copied text is somehow processed/modified and the result overwrites the selected text in the editor.  
tool_python intends to enable users of CLCL to write such plugin in python and configure them.
 
## Build the plugin
CLCL's plugin tool_python.dll can be built with the current version Python 3.14.5 (32-bit).
In order to build it, you need to have the python C header files and binary libraries at the appropriate places.  
Python on Windows systems allows for several versions side by side as subfolders of `%LOCALAPPDATA%\python`.  
 
Install the necessary version with
```cmd
pymanager isntall 3.14-32
```

## Install tool_python
- Unpack `tool_python.zip` into the same folder as `clcl.exe` (Default is C:\Program Files (x86)\clcl).  
- Download [Windows embeddable package (32-bit)](https://www.python.org/ftp/python/3.14.5/python-3.14.5-embed-win32.zip).
- Create a subfolder `python314` and unpack the zip file into that folder. _Don't_ create a sub-subfolder python-3.14.5-embed-win32 when unpacking!
- Copy `python3.dll` and `python314.dll` from the subfolder python314 into the folder of `clcl.exe`.  

That's it. Now you are ready to write and configure plugin functions in python.

## Configure python plugins
tool_python comes with an example module _tools.py_:
```python
# Python plugin functions must take string as 
# input parameter and return a string:

def main(input_text: str = ""):
    ret = "Hello Python!"
    return ret # return to CLCL

def ToLower(s: str = ""):
    return s.lower()

def ToUpper(s: str = ""):
    return s.upper()
```
These functions are not very fancy, but will give you the idea.

To configure these functions as tools in CLCL
- open the _Options_ window
- activate the _Tools_ tab
- click on the button _Add_
- in the new opened dialog click on _Browse_ 
- navigate to your folder and select `tool_python.dll`
- chose the one and only function cb_python0
- you should change the title to something meaningful, e.g. "Convert to UPPERCASE".
- click on OK
- scroll to the bottom of the plugin list and select your newly created plugin function
- now click on the _Properties_ button
- in the dialog "Select Python Module and Function" click on _Browse_ and select your python module, e.g. sc2 (the module must be in the same folder as `tool_python.dll` or CLCL's ini folder %LOCALAPPDATA%\clcl.
- select one of the modules functions from the combo box, e.g. "ToUpper" (function must have exactly one parameter of type `str` and result of type `str`
- click button _OK_
- in the _Tools_ tab of the _Options_ you can continue to add more Python functions
- when finished click _OK_ in the Options dialog  

Now CLCL is ready with all the new Python functions.  
