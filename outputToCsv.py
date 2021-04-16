import sys
import os.path
import os
import time
import csv

def parseData(readFile,csvw):
    cycle_count = 0
    ID = 0
    Inst = "inst"
    Result = 'res'
    Exp = 0
    Act = 0

    for line in readFile:
        # cycle counter line
        if 'ExitCode' not in line:
            line = line.split(' ')
            cycle_count = int(line[0]) - 2
            
        else:
            line = line.split()
            ID = line[0]
            Inst = line[1]
            Result = line[2]
            Exp = line[5]
            Act = line[8]
            csvw.writerow([str(ID),str(Inst),str(Result),str(Exp),str(Act),str(cycle_count)])

def write_csv():

    filename = str(sys.argv[1]) + "_result.csv"
    csvwr = open(filename, "w")
    csvw = csv.writer(csvwr)
    csvw.writerow(["ID","Instruction","Result","Expected ExitCode","Actual ExitCode","Cycles"])

    readFile = open("output.txt", "r")

    parseData(readFile,csvw)

    csvwr.close()


if __name__ == "__main__": 
  
    # calling main function 
    write_csv()