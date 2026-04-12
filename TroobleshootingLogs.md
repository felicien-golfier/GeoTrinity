**Enter here all your issues and fixes you got when setup this project, upgrade Unreal engine etc..**

# UE 5.7.2
30/01/2026 

commit 562880e0ffc015ddce292007cde1ac44ac83389e 

	- Had to update manually Source/GeoTrinity.Target.cs and Source/GeoTrinityEditor.Target.cs 
	- BitFlags needs now the full Enum path (ex : /Script/GeoTrinity.ETeam")
	- /!\ Loaded Failed on my Solution in Rider : Had to install the new Visual Studio with .Net 8.0 MANUALLY.
	- Forgot to download the pdb for Unreal in the launcher. 


# ExecCalc attribute capture: Source vs Target
When adding a new ExecCalc that captures a multiplier attribute (e.g. HealMultiplier), capture from **Target** if the GE is applied by a non-character source (deployable, pickup). Capture from **Source** only when the caster/attacker is the source and owns the attribute (e.g. DamageMultiplier on the attacker).
If `FindCaptureSpecByDefinition` returns null with `bOnlyIncludeValidCapture=true`, the capture spec exists but `HasValidCapture()` is false — the source ASC doesn't own the attribute set containing that attribute.



Notes from Oliver : 
- After each pull, open the cmd
- in the cmd, paste : "YourLocalUnrealInstallPath\Engine\Build\BatchFiles\Build.bat" GeoTrinityEditor Win64 Development "TheLocationOfThe .uproject"
- should look like this :  "C:\Program Files\Epic Games\UE_5.3\Engine\Build\BatchFiles\Build.bat"  GeoTrinityEditor Win64 Development  "C:\Dev\MyProject\MyProject.uproject"