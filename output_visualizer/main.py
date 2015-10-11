from getAllWaggles import getAllWaggles
from tkinter import Tk
from tkinter.filedialog import askopenfile as openfile
from subprocess import Popen
from getAllClusters import getAllClusters

def writeAllWagglesIntoCSV(waggles=None, resetStart=False):
    def h(w):
        return(w.positions[0][2])
    if(waggles==None):
        waggles = getAllWaggles()
    root = Tk()
    root.withdraw()
    file = openfile('w')
    root.destroy()
    #file.write("x y z\n")
    firstDance = 0
    if(resetStart):
        min(map(h, waggles))
    for w in waggles:
        positions = w.positions
        for p in positions[0:1]:
            file.write(str(p[0])+' '+str(p[1])+' '+str(p[2]-firstDance)+'\n')
    file.flush()
    file.close()

def writeAllClustersIntoCSV(waggles=None, resetStart=False):
    def h(w):
        return(w.positions[0][2])
    if(waggles==None):
        waggles = getAllWaggles()
    clusters = getAllClusters(waggles);
    root = Tk()
    root.withdraw()
    file = openfile('w')
    root.destroy()
    #file.write("x y z\n")
    firstDance = 0
    if(resetStart):
        min(map(h, waggles))
    for c in clusters:
        for w in c:
            p = w.positions[0]
            file.write(str(p[0])+' '+str(p[1])+' '+str(p[2]-firstDance)+' '+str(w.arraypos)+'\n')
        file.write("\n\n")
    file.flush()
    file.close()

def printWagglesAndClusters():
    waggles = getAllWaggles()
    writeAllWagglesIntoCSV(waggles)
    writeAllClustersIntoCSV(waggles)
    Popen(["gnuplot","visualize.gp"])
    
printWagglesAndClusters()
