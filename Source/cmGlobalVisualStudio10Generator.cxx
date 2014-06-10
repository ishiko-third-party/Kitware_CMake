/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2000-2009 Kitware, Inc., Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "windows.h" // this must be first to define GetCurrentDirectory
#include "cmGlobalVisualStudio10Generator.h"
#include "cmLocalVisualStudio10Generator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmVisualStudioSlnData.h"
#include "cmVisualStudioSlnParser.h"
#include "cmake.h"

static const char vs10generatorName[] = "Visual Studio 10 2010";

// Map generator name without year to name with year.
static const char* cmVS10GenName(const std::string& name, std::string& genName)
{
  if(strncmp(name.c_str(), vs10generatorName,
             sizeof(vs10generatorName)-6) != 0)
    {
    return 0;
    }
  const char* p = name.c_str() + sizeof(vs10generatorName) - 6;
  if(cmHasLiteralPrefix(p, " 2010"))
    {
    p += 5;
    }
  genName = std::string(vs10generatorName) + p;
  return p;
}

class cmGlobalVisualStudio10Generator::Factory
  : public cmGlobalGeneratorFactory
{
public:
  virtual cmGlobalGenerator* CreateGlobalGenerator(
                                            const std::string& name) const
    {
    std::string genName;
    const char* p = cmVS10GenName(name, genName);
    if(!p)
      { return 0; }
    if(!*p)
      {
      return new cmGlobalVisualStudio10Generator(
        genName, "", "");
      }
    if(*p++ != ' ')
      { return 0; }
    if(strcmp(p, "Win64") == 0)
      {
      return new cmGlobalVisualStudio10Generator(
        genName, "x64", "CMAKE_FORCE_WIN64");
      }
    if(strcmp(p, "IA64") == 0)
      {
      return new cmGlobalVisualStudio10Generator(
        genName, "Itanium", "CMAKE_FORCE_IA64");
      }
    return 0;
    }

  virtual void GetDocumentation(cmDocumentationEntry& entry) const
    {
    entry.Name = vs10generatorName;
    entry.Brief = "Generates Visual Studio 10 (VS 2010) project files.";
    }

  virtual void GetGenerators(std::vector<std::string>& names) const
    {
    names.push_back(vs10generatorName);
    names.push_back(vs10generatorName + std::string(" IA64"));
    names.push_back(vs10generatorName + std::string(" Win64"));
    }
};

//----------------------------------------------------------------------------
cmGlobalGeneratorFactory* cmGlobalVisualStudio10Generator::NewFactory()
{
  return new Factory;
}

//----------------------------------------------------------------------------
cmGlobalVisualStudio10Generator::cmGlobalVisualStudio10Generator(
  const std::string& name, const std::string& platformName,
  const std::string& additionalPlatformDefinition)
  : cmGlobalVisualStudio8Generator(name, platformName,
                                   additionalPlatformDefinition)
{
  std::string vc10Express;
  this->ExpressEdition = cmSystemTools::ReadRegistryValue(
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\10.0\\Setup\\VC;"
    "ProductDir", vc10Express, cmSystemTools::KeyWOW64_32);
  this->MasmEnabled = false;
  this->MSBuildCommandInitialized = false;
}

//----------------------------------------------------------------------------
bool
cmGlobalVisualStudio10Generator::MatchesGeneratorName(
                                               const std::string& name) const
{
  std::string genName;
  if(cmVS10GenName(name, genName))
    {
    return genName == this->GetName();
    }
  return false;
}

//----------------------------------------------------------------------------
bool
cmGlobalVisualStudio10Generator::SetGeneratorToolset(std::string const& ts,
                                                     cmMakefile* mf)
{
  this->GeneratorToolset = ts;
  this->AddVSPlatformToolsetDefinition(mf);
  return true;
}

//----------------------------------------------------------------------------
void cmGlobalVisualStudio10Generator::AddPlatformDefinitions(cmMakefile* mf)
{
  cmGlobalVisualStudio8Generator::AddPlatformDefinitions(mf);
  this->AddVSPlatformToolsetDefinition(mf);
}

//----------------------------------------------------------------------------
void cmGlobalVisualStudio10Generator
::AddVSPlatformToolsetDefinition(cmMakefile* mf) const
{
  if(const char* toolset = this->GetPlatformToolset())
    {
    mf->AddDefinition("CMAKE_VS_PLATFORM_TOOLSET", toolset);
    }
}

//----------------------------------------------------------------------------
void cmGlobalVisualStudio10Generator::WriteSLNHeader(std::ostream& fout)
{
  fout << "Microsoft Visual Studio Solution File, Format Version 11.00\n";
  if (this->ExpressEdition)
    {
    fout << "# Visual C++ Express 2010\n";
    }
  else
    {
    fout << "# Visual Studio 2010\n";
    }
}

///! Create a local generator appropriate to this Global Generator
cmLocalGenerator *cmGlobalVisualStudio10Generator::CreateLocalGenerator()
{
  cmLocalVisualStudio10Generator* lg =
    new cmLocalVisualStudio10Generator(cmLocalVisualStudioGenerator::VS10);
  lg->SetPlatformName(this->GetPlatformName());
  lg->SetGlobalGenerator(this);
  return lg;
}

//----------------------------------------------------------------------------
void cmGlobalVisualStudio10Generator::Generate()
{
  this->LongestSource = LongestSourcePath();
  this->cmGlobalVisualStudio8Generator::Generate();
  if(this->LongestSource.Length > 0)
    {
    cmMakefile* mf = this->LongestSource.Target->GetMakefile();
    cmOStringStream e;
    e <<
      "The binary and/or source directory paths may be too long to generate "
      "Visual Studio 10 files for this project.  "
      "Consider choosing shorter directory names to build this project with "
      "Visual Studio 10.  "
      "A more detailed explanation follows."
      "\n"
      "There is a bug in the VS 10 IDE that renders property dialog fields "
      "blank for files referenced by full path in the project file.  "
      "However, CMake must reference at least one file by full path:\n"
      "  " << this->LongestSource.SourceFile->GetFullPath() << "\n"
      "This is because some Visual Studio tools would append the relative "
      "path to the end of the referencing directory path, as in:\n"
      "  " << mf->GetCurrentOutputDirectory() << "/"
      << this->LongestSource.SourceRel << "\n"
      "and then incorrectly complain that the file does not exist because "
      "the path length is too long for some internal buffer or API.  "
      "To avoid this problem CMake must use a full path for this file "
      "which then triggers the VS 10 property dialog bug.";
    mf->IssueMessage(cmake::WARNING, e.str().c_str());
    }
}

//----------------------------------------------------------------------------
void cmGlobalVisualStudio10Generator
::EnableLanguage(std::vector<std::string>const &  lang,
                 cmMakefile *mf, bool optional)
{
  if(this->PlatformName == "Itanium" || this->PlatformName == "x64")
    {
    if(this->IsExpressEdition() && !this->Find64BitTools(mf))
      {
      return;
      }
    }

  for(std::vector<std::string>::const_iterator it = lang.begin();
      it != lang.end(); ++it)
    {
    if(*it == "ASM_MASM")
      {
      this->MasmEnabled = true;
      }
    }

  cmGlobalVisualStudio8Generator::EnableLanguage(lang, mf, optional);
}

//----------------------------------------------------------------------------
const char* cmGlobalVisualStudio10Generator::GetPlatformToolset() const
{
  if(!this->GeneratorToolset.empty())
    {
    return this->GeneratorToolset.c_str();
    }
  if(!this->DefaultPlatformToolset.empty())
    {
    return this->DefaultPlatformToolset.c_str();
    }
  return 0;
}

//----------------------------------------------------------------------------
std::string cmGlobalVisualStudio10Generator::GetUserMacrosDirectory()
{
  std::string base;
  std::string path;

  // base begins with the VisualStudioProjectsLocation reg value...
  if (cmSystemTools::ReadRegistryValue(
    "HKEY_CURRENT_USER\\Software\\Microsoft\\VisualStudio\\10.0;"
    "VisualStudioProjectsLocation",
    base))
    {
    cmSystemTools::ConvertToUnixSlashes(base);

    // 9.0 macros folder:
    path = base + "/VSMacros80";
      // *NOT* a typo; right now in Visual Studio 2008 beta the macros
      // folder is VSMacros80... They may change it to 90 before final
      // release of 2008 or they may not... we'll have to keep our eyes
      // on it
    }

  // path is (correctly) still empty if we did not read the base value from
  // the Registry value
  return path;
}

//----------------------------------------------------------------------------
std::string cmGlobalVisualStudio10Generator::GetUserMacrosRegKeyBase()
{
  return "Software\\Microsoft\\VisualStudio\\10.0\\vsmacros";
}

//----------------------------------------------------------------------------
void cmGlobalVisualStudio10Generator::FindMakeProgram(cmMakefile* mf)
{
  this->cmGlobalVisualStudio8Generator::FindMakeProgram(mf);
  mf->AddDefinition("CMAKE_VS_MSBUILD_COMMAND",
                    this->GetMSBuildCommand().c_str());
}

//----------------------------------------------------------------------------
std::string const& cmGlobalVisualStudio10Generator::GetMSBuildCommand()
{
  if(!this->MSBuildCommandInitialized)
    {
    this->MSBuildCommandInitialized = true;
    this->MSBuildCommand = this->FindMSBuildCommand();
    }
  return this->MSBuildCommand;
}

//----------------------------------------------------------------------------
std::string cmGlobalVisualStudio10Generator::FindMSBuildCommand()
{
  std::string msbuild;
  std::string mskey =
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\";
  mskey += this->GetToolsVersion();
  mskey += ";MSBuildToolsPath";
  if(cmSystemTools::ReadRegistryValue(mskey.c_str(), msbuild,
                                      cmSystemTools::KeyWOW64_32))
    {
    cmSystemTools::ConvertToUnixSlashes(msbuild);
    msbuild += "/";
    }
  msbuild += "MSBuild.exe";
  return msbuild;
}

//----------------------------------------------------------------------------
std::string cmGlobalVisualStudio10Generator::FindDevEnvCommand()
{
  if(this->ExpressEdition)
    {
    // Visual Studio Express >= 10 do not have "devenv.com" or
    // "VCExpress.exe" that we can use to build reliably.
    // Tell the caller it needs to use MSBuild instead.
    return "";
    }
  // Skip over the cmGlobalVisualStudio8Generator implementation because
  // we expect a real devenv and do not want to look for VCExpress.
  return this->cmGlobalVisualStudio71Generator::FindDevEnvCommand();
}

//----------------------------------------------------------------------------
void cmGlobalVisualStudio10Generator::GenerateBuildCommand(
  std::vector<std::string>& makeCommand,
  const std::string& makeProgram,
  const std::string& projectName,
  const std::string& projectDir,
  const std::string& targetName,
  const std::string& config,
  bool fast,
  std::vector<std::string> const& makeOptions)
{
  // Select the caller- or user-preferred make program, else MSBuild.
  std::string makeProgramSelected =
    this->SelectMakeProgram(makeProgram, this->GetMSBuildCommand());

  // Check if the caller explicitly requested a devenv tool.
  std::string makeProgramLower = makeProgramSelected;
  cmSystemTools::LowerCase(makeProgramLower);
  bool useDevEnv =
    (makeProgramLower.find("devenv") != std::string::npos ||
     makeProgramLower.find("vcexpress") != std::string::npos);

  // MSBuild is preferred (and required for VS Express), but if the .sln has
  // an Intel Fortran .vfproj then we have to use devenv. Parse it to find out.
  cmSlnData slnData;
  {
  std::string slnFile;
  if(!projectDir.empty())
    {
    slnFile = projectDir;
    slnFile += "/";
    }
  slnFile += projectName;
  slnFile += ".sln";
  cmVisualStudioSlnParser parser;
  if(parser.ParseFile(slnFile, slnData,
                      cmVisualStudioSlnParser::DataGroupProjects))
    {
    std::vector<cmSlnProjectEntry> slnProjects = slnData.GetProjects();
    for(std::vector<cmSlnProjectEntry>::iterator i = slnProjects.begin();
        !useDevEnv && i != slnProjects.end(); ++i)
      {
      std::string proj = i->GetRelativePath();
      if(proj.size() > 7 &&
         proj.substr(proj.size()-7) == ".vfproj")
        {
        useDevEnv = true;
        }
      }
    }
  }
  if(useDevEnv)
    {
    // Use devenv to build solutions containing Intel Fortran projects.
    cmGlobalVisualStudio7Generator::GenerateBuildCommand(
      makeCommand, makeProgram, projectName, projectDir,
      targetName, config, fast, makeOptions);
    return;
    }

  makeCommand.push_back(makeProgramSelected);

  std::string realTarget = targetName;
  // msbuild.exe CxxOnly.sln /t:Build /p:Configuration=Debug /target:ALL_BUILD
  if(realTarget.empty())
    {
    realTarget = "ALL_BUILD";
    }
  if ( realTarget == "clean" )
    {
    makeCommand.push_back(std::string(projectName)+".sln");
    makeCommand.push_back("/t:Clean");
    }
  else
    {
    std::string targetProject(realTarget);
    targetProject += ".vcxproj";
    if (targetProject.find('/') == std::string::npos)
      {
      // it might be in a subdir
      if (cmSlnProjectEntry const* proj =
          slnData.GetProjectByName(realTarget))
        {
        targetProject = proj->GetRelativePath();
        cmSystemTools::ConvertToUnixSlashes(targetProject);
        }
      }
    makeCommand.push_back(targetProject);
    }
  std::string configArg = "/p:Configuration=";
  if(!config.empty())
    {
    configArg += config;
    }
  else
    {
    configArg += "Debug";
    }
  makeCommand.push_back(configArg);
  makeCommand.push_back(std::string("/p:VisualStudioVersion=")+
                        this->GetIDEVersion());
  makeCommand.insert(makeCommand.end(),
                     makeOptions.begin(), makeOptions.end());
}

//----------------------------------------------------------------------------
bool cmGlobalVisualStudio10Generator::Find64BitTools(cmMakefile* mf)
{
  if(this->GetPlatformToolset())
    {
    return true;
    }
  // This edition does not come with 64-bit tools.  Look for them.
  //
  // TODO: Detect available tools?  x64\v100 exists but does not work?
  // KHLM\\SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\4.0;VCTargetsPath
  // c:/Program Files (x86)/MSBuild/Microsoft.Cpp/v4.0/Platforms/
  //   {Itanium,Win32,x64}/PlatformToolsets/{v100,v90,Windows7.1SDK}
  std::string winSDK_7_1;
  if(cmSystemTools::ReadRegistryValue(
       "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\"
       "Windows\\v7.1;InstallationFolder", winSDK_7_1))
    {
    cmOStringStream m;
    m << "Found Windows SDK v7.1: " << winSDK_7_1;
    mf->DisplayStatus(m.str().c_str(), -1);
    this->DefaultPlatformToolset = "Windows7.1SDK";
    return true;
    }
  else
    {
    cmOStringStream e;
    e << "Cannot enable 64-bit tools with Visual Studio 2010 Express.\n"
      << "Install the Microsoft Windows SDK v7.1 to get 64-bit tools:\n"
      << "  http://msdn.microsoft.com/en-us/windows/bb980924.aspx";
    mf->IssueMessage(cmake::FATAL_ERROR, e.str().c_str());
    cmSystemTools::SetFatalErrorOccured();
    return false;
    }
}

//----------------------------------------------------------------------------
std::string
cmGlobalVisualStudio10Generator
::GenerateRuleFile(std::string const& output) const
{
  // The VS 10 generator needs to create the .rule files on disk.
  // Hide them away under the CMakeFiles directory.
  std::string ruleDir = this->GetCMakeInstance()->GetHomeOutputDirectory();
  ruleDir += cmake::GetCMakeFilesDirectory();
  ruleDir += "/";
  ruleDir += cmSystemTools::ComputeStringMD5(
    cmSystemTools::GetFilenamePath(output).c_str());
  std::string ruleFile = ruleDir + "/";
  ruleFile += cmSystemTools::GetFilenameName(output);
  ruleFile += ".rule";
  return ruleFile;
}

//----------------------------------------------------------------------------
void cmGlobalVisualStudio10Generator::PathTooLong(
  cmTarget* target, cmSourceFile const* sf, std::string const& sfRel)
{
  size_t len = (strlen(target->GetMakefile()->GetCurrentOutputDirectory()) +
                1 + sfRel.length());
  if(len > this->LongestSource.Length)
    {
    this->LongestSource.Length = len;
    this->LongestSource.Target = target;
    this->LongestSource.SourceFile = sf;
    this->LongestSource.SourceRel = sfRel;
    }
}

//----------------------------------------------------------------------------
bool cmGlobalVisualStudio10Generator::UseFolderProperty()
{
  return IsExpressEdition() ? false : cmGlobalGenerator::UseFolderProperty();
}
