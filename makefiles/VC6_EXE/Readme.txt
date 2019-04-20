INSTRUCTIONS FOR COMPILING EPANET2.EXE USING MICROSOFT VISUAL C/C++ 6.0
=======================================================================

1. Open the file EPANET.C in a text editor and make sure that the line
       #define CLE
   is not commented out while the lines
       #define DLL
       #define SOL
   are commented out.

2. Create a sub-directory named EPANET2_EXE under the directory where
   the EPANET Engine source code files are stored and copy EPANET2.MAK
   to it.

3. Open a command line window and navigate to the EPANET2_EXE directory.

4. Issue the command:

      nmake /f EPANET2.MAK

   to create EPANET2.EXE which will can be found in the EPANET2_EXE\Release
   directory.

Note: In the official distribution of EPANET for Windows, the command line
      executable version of EPANET is named EPANET2D.EXE and is referred to
      as such in the Users Manual.

