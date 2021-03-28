# Image_Comparator  

### A simple C++ program that will search on a directory and all sub-directories images and check for repeated ones, works even on different resolution images!

#### Index
[Summary](#summary)  
[How to use](#how-to-use)  
[Config](#config)  
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
5- Enjoy the beautiful progress bar   
  
#### Config
After release 2.1 there's a config file (config.cfg) where the user can change 2 things, choose whether or not to use OpenCL and hte number of threads to use, you can change them by accessing the file and changing the yes for a no or the default number of threads for another but keep in mind a few things:  
1- Do not delete the # of the comment since it will remembe future you if you need to change it (also the program needs it to parse the input)
2- Do not put any space after the =
  
#### To-do
Right now I'm working on making the code Linux compatible, if you're gonna ask for Mac compatibility do it yourself I have no idea of Mac's things
  
#### Credits  
Special thanks to my friend KiwiRives who helped polish from the first iterations. The code was created by me, [Joan Gil](https://www.linkedin.com/in/joan-gil-rigo-a65536184/) using the libraries: OpenCV and Boost (because Windows sucks).  
  
#### Donations  
If you're interested in donating to me (somehow) here's a link to [paypal](https://www.paypal.me/jgil99).
