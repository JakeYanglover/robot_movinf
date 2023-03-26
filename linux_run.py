import subprocess
import shutil
import os

print("Compiling program...")
subprocess.run("g++ main.cpp -o main", shell=True)

print("Moving file...")
try:
    os.remove("./LinuxRelease/main")
except:
    pass

shutil.move("./main", "./LinuxRelease")
os.chdir("./LinuxRelease")

print("Running test...")
# Custom
subprocess.run(
        r"./main",
        shell=True)
