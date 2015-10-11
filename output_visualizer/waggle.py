from math import floor

class waggle:
    'Beinhaltet alle Daten aus einer CSV des WaggleDanceDetektor Projektes'

    def __init__(self, lines):
        for i in range(len(lines)):
            #Entferne das letzte Zeichen (\n)
            lines[i] = lines[i][:len(lines[i])-1]
            lines[i] = lines[i].split(' ')
        self.direction = float(lines[0][2])
        
        times = lines[1][0].split(':')
        self.begin = int(times[0])
        self.begin = self.begin*60   + int(times[1])
        self.begin = self.begin*60   + int(times[2])
        self.begin = self.begin*1000 + int(times[3])

        self.frames = int(lines[1][1])

        self.duration = floor(self.frames*1000/102)
        self.end = self.begin + self.duration

        self.corners = [[int(lines[2][0]),int(lines[2][1])],[int(lines[2][2]),int(lines[2][3])],[int(lines[2][4]),int(lines[2][5])],[int(lines[2][6]),int(lines[2][7])]]

        self.positions = []
        for i in range(0, len(lines[3])-1, 2):
            self.positions+=[[float(lines[3][i]),float(lines[3][i+1]), floor(self.begin+(i//2)*(1000/102))]]

    def PositionBegin(self):
        return(self.positions[0])
    def PositionEnd(self):
        return(self.position[-1])
    def TimeBegin(self):
        return(self.begin)
    def TimeEnd(self):
        return(self.end)
