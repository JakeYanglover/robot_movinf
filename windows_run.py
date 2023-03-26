import subprocess
import shutil
import os

print("Compiling program...")
subprocess.run("g++ main.cpp -o main.exe", shell=True)

print("Moving file...")
try:
    os.remove("./WindowsRelease/main.exe")
except:
    pass

shutil.move("./main.exe", "./WindowsRelease")
os.chdir("./WindowsRelease")

print("Running test...")
# Custom
subprocess.run(
        r".\main.exe",
        shell=True)
# Demo
# subprocess.run(
#         r"robot_gui.exe .\Demo\SimpleDemo.exe -m .\maps\1.txt",
#         shell=True)
