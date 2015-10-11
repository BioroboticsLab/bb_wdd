from getFilesFromFolder import getFilesFromFolder
from waggle import waggle

def getAllWaggles(root_folder=None):
    ret = []
    files = getFilesFromFolder(root_folder, [".csv"])
    for file in files:
        handle = open(file, 'r')
        lines = handle.readlines()
        ret+=[waggle(lines)]
    return(ret)
