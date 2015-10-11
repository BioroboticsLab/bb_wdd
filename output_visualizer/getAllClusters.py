from math import sqrt
from functools import partial
from getAllWaggles import getAllWaggles
from waggle import waggle
from copy import deepcopy

""" returns a list of list containing sequenzes that match the criteria """
def getAllClusters(waggles=None):
    minSeqLen = 4
    maxDistXY = 20
    maxDistT  = (1000) * 5
    minDistT  = 500
    
    if(waggles==None):
        waggles = getAllWaggles()
    firstDanceBegin = min(map(waggle.TimeBegin, waggles))
    lastDanceBegin = max(map(waggle.TimeBegin, waggles))
    time = lastDanceBegin - firstDanceBegin

    timeSlotsNum = 1+time//maxDistT
    timeSlots = [[] for i in range(timeSlotsNum+1)]#+1 damit python nicht round robin macht...

    #Give Every Waggle the ID of the Array
    for i in range(len(waggles)):
        waggles[i].arraypos = i;
        waggles[i].timeSlot = (waggles[i].TimeBegin()-firstDanceBegin)//maxDistT

    #Put all the waggles into the corresponding time slots
    for w in waggles:
        timeSlots[w.timeSlot] += [w]
    
    #Look for every waggle if there is a chain with at least length >= seqlength
    def delete(at, i):
        return at[:i]+at[i+1:]
    def dist(x1,y1,x2,y2):
        return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2))
    def match(w1, w2):
        if(w1==w2 or w1.arraypos==w2.arraypos):
            return False
        t1 = w1.TimeBegin()
        t2 = w2.TimeBegin()
        p1 = w1.PositionBegin()
        p2 = w2.PositionBegin()
        return(minDistT<t2-t1<=maxDistT and dist(p1[0], p1[1], p2[0], p2[1])<=maxDistXY)
    
    foundSeq = []
    for w in waggles:
        w.visited=False
    def rec_search(w, seq, acc):
        possiblewaggles = timeSlots[w.timeSlot]+timeSlots[w.timeSlot+1]
        seq_con = filter(partial(match, w), possiblewaggles)
        seq_con_length = 0
        for wag in seq_con:
            seq_con_length+=1
            rec_search(wag, seq+[wag], acc)
        if seq_con_length==0:
            if(len(seq)>=minSeqLen):
                acc += [deepcopy(seq)]
        w.visited=True
    for w in waggles:
        if(w.visited==False):
            rec_search(w, [w], foundSeq)

    return(foundSeq)



    
