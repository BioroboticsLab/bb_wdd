from os import sep  as sep
from os import listdir as listdir
from os.path import isfile as isfile
from os.path import isdir as isdir
from tkinter.filedialog import askdirectory as askdirectory
from tkinter import Tk as Tk

def getFilesFromFolder(folder=None, endings=[]):
    def help(dir):
        ret = []
        stuff = listdir(dir)
        for st in stuff:
            st = dir+sep+st
            if(isfile(st)):
                if len(endings)==0 or any(map(st.endswith,endings)):
                    ret+=[st]
            if(isdir(st)):
                ret+=help(st)
        return(ret)
    if(folder==None):
        root = Tk()
        root.withdraw()
        folder = askdirectory()
        root.destroy()
    return(help(folder))
