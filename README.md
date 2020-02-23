# Handout 2 Siemens Spring School 2020

# Help for installation
<ul>
<li>1)First you need to create a directory (optionally named workspace, in which you will do the next steps)<br />
(Don't have ANY spaces in any directory in the project)</li>
<li>2)Clone the repo in a new folder inside the first created directory (you should name the folder "Handout2" for it to work properly)</li>
<li>3)Do this tutorial http://api.projectchrono.org/tutorial_install_chrono.html <br />
(When folowing the instructions you need to put all the libraries and the chrono_build in the same folder created at the start) </li>
<li>4) Use cmake on the application folder and build it in a build folder <br />
("Where is the source code:" should be something like ".../workspace/Handout2" and "Where to build the binaries:" should be something like ".../workspace/Handout2/build")<br />
(If not found, edit the value of Chrono_Dir to .../workspace/chrono_build/cmake)</li>
<li>5)Go where the application was built and open the Handout2.sln</li>
<li>6)In the solution explorer right click the Handout2 project and then click "Set as StartUp project"</li>
<li>7)Change the sloution configuration to Release and the solution platform to x64</li>
</ul>
# Additional info:
-Do not copy builded directories from one place to another, rebuild them in the directory needed.




