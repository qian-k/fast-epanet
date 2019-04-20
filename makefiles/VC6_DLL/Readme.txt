INSTRUCTIONS FOR COMPILING EPANET2.DLL USING MICROSOFT VISUAL C/C++ 6.0
=======================================================================

1. Open the file EPANET.C in a text editor and make sure that the line
       #define DLL
   is not commented out while the lines
       #define CLE
       #define SOL
   are commented out.

2. Create a sub-directory named EPANET2_DLL under the directory where
   the EPANET Engine source code files are stored and copy EPANET2.MAK
   to it.

3. Open a command line window and navigate to the EPANET2_DLL directory.

4. Issue the command:

      nmake /f EPANET2.MAK

   to create EPANET2.DLL which will can be found in the EPANET2_DLL\Release
   directory.

