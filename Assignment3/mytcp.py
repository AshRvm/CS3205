import sys
import random
import matplotlib.pyplot as plt
from math import ceil

random.seed(8)

def Slow_Start(cw_old, km, mss, rws):
    return min(cw_old + km * mss, rws)

def Congestion_Avoidance(cw_old, kn, mss, rws):
    return min(cw_old + (kn * mss * mss)/cw_old, rws)

def Congestion_Detection(cw_old, kf): 
    return max(1, cw_old * kf)

def is_ACK_Recieved(prob):
    return (random.random() <= prob)

def Slow_Phase(threshold, cw, km, kf, mss, rws, ps, num_segments_left, update_list): 
    while((cw < threshold) and (num_segments_left > 0)) : 
        num_segments_sent = ceil(cw/mss)
        flag = 0
        for i in range(num_segments_sent) : 
            is_successful = is_ACK_Recieved(ps)
            if(is_successful) : 
                cw = Slow_Start(cw, km, mss, rws)
                update_list.append(cw)
                num_segments_left -= 1
            else : 
                threshold = cw/2
                cw = Congestion_Detection(cw, kf)
                update_list.append(cw)
                flag = 1
                break
        if(flag == 1) :
            break
    return [threshold, cw, num_segments_left, update_list]

def Avoidance_Phase(threshold, cw, kn, kf, mss, rws, ps, num_segments_left, update_list):
    while(num_segments_left > 0) : 
        num_segments_sent = ceil(cw/mss)
        flag = 0
        for i in range(num_segments_sent) : 
            is_successful = is_ACK_Recieved(ps)
            if(is_successful) :
                cw = Congestion_Avoidance(cw, kn, mss, rws)
                update_list.append(cw)
            else : 
                threshold = cw/2
                cw = Congestion_Detection(cw, kf)
                update_list.append(cw)
                flag = 1
                break
        if(flag == 0) : 
            num_segments_left -= num_segments_sent
        else : 
            break
    return [threshold, cw, num_segments_left, update_list]

if __name__ == "__main__":
    n = len(sys.argv)
    rws = 1024.0
    threshold = rws/2
    mss = 1.0
    cw = 1.0
    ki = 1.0
    km = 1.0
    kn = 0.5
    kf = 0.1
    
    for i in range(1,n,2) : 
        if(sys.argv[i] == "-i") : 
            ki = float(sys.argv[i+1])
            cw = ki * mss
        elif(sys.argv[i] == "-n") : 
            kn = float(sys.argv[i+1])
        elif(sys.argv[i] == "-m") : 
            km = float(sys.argv[i+1])
        elif(sys.argv[i] == "-f") : 
            kf = float(sys.argv[i+1])
        elif(sys.argv[i] == "-s") : 
            ps = float(sys.argv[i+1])
        elif(sys.argv[i] == "-T") : 
            T = float(sys.argv[i+1])
        elif(sys.argv[i] == "-o") : 
            fileName = sys.argv[i+1]
        # elif(sys.argv[i] == "-name") : 
        #     graphName = sys.argv[i+1]
    
    file = open(fileName, "w")

    num_segments_left = T
    update_list = []
    while(num_segments_left > 0) :
        while(cw < threshold) : 
            threshold, cw, num_segments_left, update_list = Slow_Phase(threshold, cw, km, kf, mss, rws, ps, num_segments_left, update_list)
        if(num_segments_left < 0) :
            break
        else :
            threshold, cw, num_segments_left, update_list = Avoidance_Phase(threshold, cw, kn, kf, mss, rws, ps, num_segments_left, update_list)
    
    update_numbers = []
    for i in range(1,len(update_list)+1) : 
        update_numbers.append(i+1)

    for i in range(len(update_list)) : 
        file.write(f"update no.{i+1}: cw = {update_list[i]}\n")
    
    plt.plot(update_numbers, update_list)
    plt.xlabel("Update Number")
    plt.ylabel("Congestion Window")
    plt.savefig(f"i={ki},m={km},n={kn},f={kf},s={ps},T={T}.png")        
    # plt.savefig(graphName)