# Image_Comparator  

### A simple C++ program that will search on a directory and all sub-directories images and check for repeated ones, works even on different resolution images!

#### Index
[Summary](#summary)  
[How to use](#how-to-use)  
[To-do](#to-do)  
[Credits](#credits)  
[Donations](#donations)
  
#### Summary  
Opens a terminal and prompts you with instructions to open a folder and start looking for repeated images inside it and all sub-folders, after that based on your selection it will delete images automatically, show the names of the files or prompt you to manually delete each of them. In case you want to compile the code you need opencv and boost filesystem, all the .dll is included in the latest release and is needed for the .exe to work. Enjoy :3  
  
#### How to use  
1- Simply download the latest releae version (unless the version is a pre-release, those are usually buggy)  
2- Unpack everything in the same folder  
3- Run the .exe file  
4- The program will ask for some inputs  
   4.1- First input: The absolute directory to the images (C:/Documents/Homework) for example  
   4.2- Second input: What to do with the images, delete based on size or weight, or manually delete or just show the path of the images  
   4.3- Tird input: Number of CPU threads to use, the more usually the faster but the sweetspot is usually your physical cores x 2 - 1 or 2, if you don't know use 4  
5- Enjoy the beautiful progress bar   
  
#### To-do
Right now I'm working on making the code Linux compatible, if you're gonna ask for Mac compatibility do it yourself I have no idea of Mac's things
  
#### Credits  
Special thanks to my friend KiwiRives who helped polish from the first iterations. The code was created by me, [Joan Gil](https://www.linkedin.com/in/joan-gil-rigo-a65536184/) using the libraries: OpenCV and Boost (because Windows sucks).  
  
#### Donations  
If you're interested in donating to me (somehow) here's a link to [paypal](https://www.paypal.me/jgil99).
