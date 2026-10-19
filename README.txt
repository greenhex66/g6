FRACTURE - Glitch plugin (VST3) for FL Studio / Windows

HOW TO BUILD THE INSTALLER (no coding, no compiler on your PC):
1. Make a free account on github.com and create a New repository (Public).
2. Click "uploading an existing file", unzip this project and drag EVERYTHING
   inside it (including the hidden-looking .github folder) into the page. Commit.
   If the .github folder does not upload: Add file > Create new file, type
   .github/workflows/build.yml as the name and paste the content of that file.
3. Open the Actions tab > "Build Fracture (Windows)" > Run workflow
   (it also starts automatically after upload). Takes ~10 minutes.
4. Open the finished run > Artifacts > download "Fracture-Windows".
   Inside: Fracture-Setup.exe (installer) and Fracture-VST3-manual.zip (backup).

IN FL STUDIO:
Options > Manage plugins > Start scan (VST3 folder: C:\Program Files\Common Files\VST3)
Then add "Fracture" on a mixer insert slot.

MANUAL INSTALL (if the .exe fails): copy Fracture.vst3 folder to
C:\Program Files\Common Files\VST3\
