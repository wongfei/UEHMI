using System;
using System.IO;
using UnrealBuildTool;

public class HMIBackend : ModuleRules
{
    public HMIBackend(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        bEnableExceptions = true;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "HMI"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "CoreUObject",
            "Engine",
            "Projects", // IPluginManager
            "WebSockets",
            "Json",
            "JsonUtilities",
        });

        // Cloud providers

        bool WithOpenAI = true; // libcurl
        bool WithElevenlabs = true; // WebSockets

        // ThirdParty providers

        bool WithCustomOpenCV = true; // Apache

        bool WithGgml = true; // MIT
        bool WithWhisper = true && WithGgml; // MIT
        bool WithLlama = true && WithGgml; // MIT

        // not compatible with UE NNERuntimeORT
        bool WithCustomOnnx = true; // MIT
        bool WithSherpa = true; // Apache + GPL
        bool WithPiper = true; // GPL

        bool WithAnyOnnx = (WithCustomOnnx || WithSherpa || WithPiper);
        bool WithFER = true && WithAnyOnnx && WithCustomOpenCV; // Apache

        bool WithOVRLipSync = true; // Oculus SDK License Agreement
        bool WithCLD2 = true; // Apache

        // ThirdPartyDir

        string ThirdPartyDir = Path.GetFullPath(Path.Combine(PluginDirectory, "Source", "ThirdParty"));

        string OverrideThirdParty = Environment.GetEnvironmentVariable("HMI_THIRDPARTY_DIR");
        if (!string.IsNullOrEmpty(OverrideThirdParty) && Directory.Exists(OverrideThirdParty))
        {
            ThirdPartyDir = OverrideThirdParty;
        }

        string BinArch = Target.Platform.ToString(); // Win64, Linux, ..
        string ThirdPartyBinDir = Path.GetFullPath(Path.Combine(PluginDirectory, "Binaries", "ThirdParty", BinArch));

        // Defines

        EnableProvider("HMI_WITH_OPENAI_CHAT", WithOpenAI);
        EnableProvider("HMI_WITH_ELEVENLABS_TTS", WithElevenlabs);

        EnableProvider("HMI_WITH_CUSTOM_OPENCV", WithCustomOpenCV);

        EnableProvider("HMI_WITH_GGML", WithGgml);
        EnableProvider("HMI_WITH_WHISPER", WithWhisper);
        EnableProvider("HMI_WITH_LLAMA", WithLlama);

        EnableProvider("HMI_WITH_CUSTOM_ONNX", WithCustomOnnx);
        EnableProvider("HMI_WITH_ANY_ONNX", WithAnyOnnx);
        EnableProvider("HMI_WITH_SHERPA", WithSherpa);
        EnableProvider("HMI_WITH_PIPER", WithPiper);
        EnableProvider("HMI_WITH_FER", WithFER);

        EnableProvider("HMI_WITH_OVRLIPSYNC", WithOVRLipSync);
        EnableProvider("HMI_WITH_CLD2", WithCLD2);

        // Include

        if (WithCustomOpenCV)
        {
            PrivateIncludePaths.Add(Path.Combine(ThirdPartyDir, "opencv", "include"));
        }

        if (WithGgml)
        {
            PrivateIncludePaths.Add(Path.Combine(ThirdPartyDir, "ggml", "include"));
        }

        if (WithWhisper)
        {
            PrivateIncludePaths.Add(Path.Combine(ThirdPartyDir, "whispercpp", "include"));
        }

        if (WithLlama)
        {
            PrivateIncludePaths.Add(Path.Combine(ThirdPartyDir, "llamacpp", "include"));
        }

        string OnnxIncludeDir = "";

        if (WithCustomOnnx) // custom compiled onnxruntime shared by all deps
        {
            OnnxIncludeDir = Path.Combine(ThirdPartyDir, "onnxruntime", "include");
        }

        if (WithSherpa)
        {
            PrivateIncludePaths.Add(Path.Combine(ThirdPartyDir, "sherpaonnx", "include"));

            if (string.IsNullOrEmpty(OnnxIncludeDir)) // use sherpa onnxruntime
            {
                OnnxIncludeDir = Path.Combine(ThirdPartyDir, "sherpaonnx", "onnxruntime", "include");
            }
        }

        if (WithPiper)
        {
            PrivateIncludePaths.Add(Path.Combine(ThirdPartyDir, "piper"));

            if (string.IsNullOrEmpty(OnnxIncludeDir)) // use piper onnxruntime, probably incompatible with sherpa
            {
                OnnxIncludeDir = Path.Combine(ThirdPartyDir, "piper", "onnxruntime", "include");
            }
        }

        if (!string.IsNullOrEmpty(OnnxIncludeDir))
        {
            PrivateIncludePaths.Add(OnnxIncludeDir);
        }

        if (WithFER && WithAnyOnnx)
        {
            PrivateDefinitions.Add("ORT_API_MANUAL_INIT"); // Ort::InitApi()
        }

        if (WithOVRLipSync)
        {
            PrivateIncludePaths.Add(Path.Combine(ThirdPartyDir, "OVRLipSync", "Include"));
        }

        if (WithCLD2)
        {
            PrivateIncludePaths.Add(Path.Combine(ThirdPartyDir, "cld2"));
            PrivateDefinitions.Add("CLD2_IMPORTS");
        }

        // Binaries

        if (!Directory.Exists(ThirdPartyBinDir))
        {
            Directory.CreateDirectory(ThirdPartyBinDir);
        }

        PublicRuntimeLibraryPaths.Add(ThirdPartyBinDir);

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            if (WithCustomOpenCV)
            {
                string Prefix = Path.Combine(ThirdPartyDir, "opencv", "x64", "vc17");
                PublicAdditionalLibraries.Add(Path.Combine(Prefix, "lib", "opencv_worldhmi.lib"));
                AddDll(ThirdPartyBinDir, Path.Combine(Prefix, "bin"), "opencv_worldhmi.dll", true);
            }

            if (WithGgml)
            {
                string Prefix = Path.Combine(ThirdPartyDir, "ggml");
                PublicAdditionalLibraries.Add(Path.Combine(Prefix, "lib", "ggml.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(Prefix, "lib", "ggml-base.lib"));

                AddDll(ThirdPartyBinDir, Path.Combine(Prefix, "bin"), "ggml.dll", true);
                AddDll(ThirdPartyBinDir, Path.Combine(Prefix, "bin"), "ggml-base.dll", true);

                // backends (dynload)
                AddDll(ThirdPartyBinDir, Path.Combine(Prefix, "bin"), "ggml-vulkan.dll");
                //AddDll(ThirdPartyBinDir, Path.Combine(Prefix, "bin"), "ggml-cuda.dll");

                var files = Directory.EnumerateFiles(Path.Combine(Prefix, "bin"), "ggml-cpu-*.dll", SearchOption.TopDirectoryOnly);
                foreach (var file in files)
                {
                    AddDll(ThirdPartyBinDir, Path.Combine(Prefix, "bin"), Path.GetFileName(file));
                }
            }

            if (WithWhisper)
            {
                string Prefix = Path.Combine(ThirdPartyDir, "whispercpp");
                PublicAdditionalLibraries.Add(Path.Combine(Prefix, "lib", "whisper.lib"));
                AddDll(ThirdPartyBinDir, Path.Combine(Prefix, "bin"), "whisper.dll", true);
            }

            if (WithLlama)
            {
                string Prefix = Path.Combine(ThirdPartyDir, "llamacpp");
                PublicAdditionalLibraries.Add(Path.Combine(Prefix, "lib", "llama.lib"));
                AddDll(ThirdPartyBinDir, Path.Combine(Prefix, "bin"), "llama.dll", true);
            }

            string OnnxBinariesDir = "";

            if (WithCustomOnnx)
            {
                OnnxBinariesDir = Path.Combine(ThirdPartyDir, "onnxruntime", "lib");
            }

            if (WithSherpa) // dynload
            {
                string Prefix = Path.Combine(ThirdPartyDir, "sherpaonnx");
                AddDll(ThirdPartyBinDir, Path.Combine(Prefix, "lib"), "sherpa-onnx-c-api.dll");

                if (string.IsNullOrEmpty(OnnxBinariesDir))
                {
                    OnnxBinariesDir = Path.Combine(Prefix, "onnxruntime", "lib");
                }
            }

            if (WithPiper) // dynload
            {
                string Prefix = Path.Combine(ThirdPartyDir, "piper");
                AddDll(ThirdPartyBinDir, Prefix, "piper.dll");
                AddDll(ThirdPartyBinDir, Prefix, "piper_phonemize.dll");
                AddDll(ThirdPartyBinDir, Prefix, "espeak-ng.dll");

                if (string.IsNullOrEmpty(OnnxBinariesDir))
                {
                    OnnxBinariesDir = Path.Combine(Prefix, "onnxruntime", "lib");
                }
            }

            if (!string.IsNullOrEmpty(OnnxBinariesDir))
            {
                AddOnnx(ThirdPartyBinDir, OnnxBinariesDir, true);

                if (WithFER)
                {
                    PublicAdditionalLibraries.Add(Path.Combine(OnnxBinariesDir, "onnxruntime.lib"));
                }
            }

            if (WithOVRLipSync) // dynload by shim
            {
                string Prefix = Path.Combine(ThirdPartyDir, "OVRLipSync", "Win64");
                PublicAdditionalLibraries.Add(Path.Combine(Prefix, "OVRLipSyncShim.lib"));
                AddDll(ThirdPartyBinDir, Prefix, "OVRLipSync.dll");
            }

            if (WithCLD2)
            {
                string Prefix = Path.Combine(ThirdPartyDir, "cld2");
                PublicAdditionalLibraries.Add(Path.Combine(Prefix, "cld2.lib"));
                AddDll(ThirdPartyBinDir, Prefix, "cld2.dll", true);
            }
        }

        // HMI_Data

        string DataDir = Path.GetFullPath(Path.Combine(PluginDirectory, "HMI_Data"));
        if (Directory.Exists(DataDir))
        {
            var files = Directory.EnumerateFiles(DataDir, "*", SearchOption.AllDirectories);
            foreach (var file in files)
            {
                RuntimeDependencies.Add(file);
            }
        }
    }

    private void EnableProvider(string Name, bool Enabled)
    {
        PublicDefinitions.Add(Name + "=" + (Enabled ? "1" : "0"));
    }

    private void AddDll(string BinaryOutputDir, string Prefix, string Name, bool DelayLoad = false)
    {
        string SrcPath = Path.Combine(Prefix, Name);
        if (!File.Exists(SrcPath))
        {
            return;
        }

        string FinalPath = Path.Combine(BinaryOutputDir, Name);
        if (!File.Exists(FinalPath) || !FilesEqual(SrcPath, FinalPath))
        {
            File.Copy(SrcPath, FinalPath, true);

            /*string SrcPdb = SrcPath.Replace(".dll", ".pdb");
            if (File.Exists(SrcPdb))
            {
                string DstPdb = FinalPath.Replace(".dll", ".pdb");
                File.Copy(SrcPdb, DstPdb, true);
            }*/
        }

        if (DelayLoad)
            PublicDelayLoadDLLs.Add(Name);

        RuntimeDependencies.Add(FinalPath);
    }

    private void AddOnnx(string ThirdPartyBinDir, string Prefix, bool DelayLoad = false)
    {
        AddDll(ThirdPartyBinDir, Prefix, "onnxruntime.dll", DelayLoad);
        AddDll(ThirdPartyBinDir, Prefix, "DirectML.dll");
        AddDll(ThirdPartyBinDir, Prefix, "DirectML.Debug.dll");
    }

    private bool FilesEqual(string filePath1, string filePath2)
    {
        FileInfo f1 = new FileInfo(filePath1);
        FileInfo f2 = new FileInfo(filePath2);

        if (f1.Length != f2.Length)
            return false;

        byte[] data1 = File.ReadAllBytes(filePath1);
        uint hash1 = Crc32(data1);

        byte[] data2 = File.ReadAllBytes(filePath2);
        uint hash2 = Crc32(data2);

        return hash1 == hash2;
    }

    private uint Crc32(byte[] bytes)
    {
        uint crc = 0xFFFFFFFF;
        for (int i = 0; i < bytes.Length; i++)
        {
            crc ^= bytes[i];
            for (int j = 0; j < 8; j++)
            {
                uint mask = (uint)-(crc & 1);
                crc = (crc >> 1) ^ (0xEDB88320 & mask);
            }
        }
        return ~crc;
    }
}
