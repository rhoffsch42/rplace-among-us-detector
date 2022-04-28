import os
import subprocess
from multiprocessing import Pool

cpu_count = os.cpu_count()
cpu_used = cpu_count // 1
detector_path = "../rplace-among-us-detector/rplace-among-us-detector.exe"
log_folder = "log/"
input_folder = "D:/DATA/rplace2022_TwitchFR_vs_ES-US/dl/images/"
files = [f for f in os.listdir(input_folder) if f.endswith(".png")]
#files = files[:cpu_used*1000]
file_count = len(files)

def startProcess(file):
    print("process id:", os.getpid(), "file:", file)
    ret = subprocess.run([detector_path, input_folder+file], capture_output=True)
    #ret = subprocess.check_output(detector_path + " " + input_folder + file)

    logfile = log_folder+"log_success.txt"
    if ret.returncode != 0:
        logfile = log_folder+"log_error.txt"
    with open(logfile, "a") as f:
        f.write("errorcode: " + str(ret.returncode))
        f.write("\nstdout:\n" + ret.stdout.decode("utf-8").replace("\r", ""))
        f.write("\nstderr:\n" + ret.stderr.decode("utf-8").replace("\r", ""))
        f.write("\n----------------------\n")

def startPools():
    if not os.path.exists(log_folder):
        os.mkdir(log_folder)
    print('cpu_count', cpu_count)
    print('cpu_used', cpu_used)
    with Pool(cpu_used) as pool:
        for i in range(0, file_count, cpu_used):
            print("images scanned:", i, "/", file_count)
            pool.map(startProcess, files[i:i+cpu_used])
     
if __name__ == '__main__':
    startPools()