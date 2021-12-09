import sys
import os

# sys.path.append("..")
dirname, filename = os.path.split(os.path.abspath(__file__)) 
sys.path.append(dirname[0:dirname.rfind("/")])
# os.environ["PYTHONPYTH"]=dirname[0:dirname.rfind("/")]