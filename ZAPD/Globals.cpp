#include "Globals.h"

#include <algorithm>
#include <string_view>

#include <Utils/DiskFile.h>
#include "Utils/Path.h"
#include "WarningHandler.h"
#include "tinyxml2.h"

Globals* Globals::Instance;

Globals::Globals()
{
	Instance = this;

	game = ZGame::OOT_RETAIL;
	genSourceFile = true;
	testMode = false;
	profile = false;
	useLegacyZDList = false;
	useExternalResources = true;
	singleThreaded = true;
	verbosity = VerbosityLevel::VERBOSITY_SILENT;
	outputPath = Directory::GetCurrentDirectory();
}

Globals::~Globals()
{
	for (const auto& w : workerData)
	{
		delete w.second;
	}
	
	if (rom != nullptr)
	{
		delete rom;
	}
}

void Globals::AddSegment(int32_t segment, ZFile* file, int workerID)
{
	if (!Globals::Instance->singleThreaded)
	{
		auto worker = workerData[workerID];

		if (std::find(worker->segments.begin(), worker->segments.end(), segment) ==
		    worker->segments.end())
			worker->segments.push_back(segment);
		if (worker->segmentRefFiles.find(segment) == worker->segmentRefFiles.end())
			worker->segmentRefFiles[segment] = std::vector<ZFile*>();

		worker->segmentRefFiles[segment].push_back(file);
	}
	else
	{
		if (std::find(segments.begin(), segments.end(), segment) == segments.end())
			segments.push_back(segment);
		if (cfg.segmentRefFiles.find(segment) == cfg.segmentRefFiles.end())
			cfg.segmentRefFiles[segment] = std::vector<ZFile*>();

		cfg.segmentRefFiles[segment].push_back(file);
	}
}

bool Globals::HasSegment(int32_t segment, int workerID)
{
	if (!Globals::Instance->singleThreaded)
		return std::find(workerData[workerID]->segments.begin(),
		                 workerData[workerID]->segments.end(), segment) != workerData[workerID]->segments.end();
	else
		return std::find(segments.begin(), segments.end(), segment) != segments.end();
}

ZFile* Globals::GetSegment(int32_t segment, int workerID)
{
	if (!Globals::Instance->singleThreaded)
	{
		if (HasSegment(segment, workerID))
		{
			int idx = std::find(workerData[workerID]->segments.begin(),
			                    workerData[workerID]->segments.end(), segment) -
			          workerData[workerID]->segments.begin();
			return workerData[workerID]->files[idx];
		}
		else
			return nullptr;
	}
	else
	{
		if (HasSegment(segment, workerID))
		{
			int idx = std::find(segments.begin(), segments.end(), segment) - segments.begin();
			return files[idx];
		}
		else
			return nullptr;
	}
}

std::map<int32_t, std::vector<ZFile*>> Globals::GetSegmentRefFiles(int workerID)
{
	if (!Globals::Instance->singleThreaded)
		return workerData[workerID]->segmentRefFiles;
	else
		return cfg.segmentRefFiles;
}

void Globals::AddFile(ZFile* file, int workerID)
{
	if (singleThreaded)
		files.push_back(file);
	else
		workerData[workerID]->files.push_back(file);
}

void Globals::AddExternalFile(ZFile* file, int workerID)
{
	if (singleThreaded)
		externalFiles.push_back(file);
	else
		workerData[workerID]->externalFiles.push_back(file);
}

void Globals::BuildAssetTexture(const fs::path& pngFilePath, TextureType texType, const fs::path& outPath)
{
	std::string name = outPath.stem().string();

	ZTexture tex(nullptr);

	if (name.find("u32") != std::string::npos)
		tex.dWordAligned = false;

	tex.FromPNG(pngFilePath.string(), texType);
	std::string cfgPath = StringHelper::Split(pngFilePath.string(), ".")[0] + ".cfg";

	if (DiskFile::Exists(cfgPath))
		name = DiskFile::ReadAllText(cfgPath);

	std::string src = tex.GetBodySourceCode();

	DiskFile::WriteAllBytes(outPath.string(), src.c_str(), src.size());
}

std::map<std::string, ExporterSet*>& Globals::GetExporterMap()
{
	static std::map<std::string, ExporterSet*> exporters;
	return exporters;
}

void Globals::AddExporter(std::string exporterName, ExporterSet* exporterSet)
{
	auto& exporters = GetExporterMap();
	exporters[exporterName] = exporterSet;
}

ZResourceExporter* Globals::GetExporter(ZResourceType resType)
{
	auto& exporters = GetExporterMap();

	if (currentExporter != "" && exporters[currentExporter]->exporters.find(resType) !=
	                                 exporters[currentExporter]->exporters.end())
		return exporters[currentExporter]->exporters[resType];
	else
		return nullptr;
}

ExporterSet* Globals::GetExporterSet()
{
	auto& exporters = GetExporterMap();

	if (currentExporter != "")
		return exporters[currentExporter];
	else
		return nullptr;
}

std::vector<uint8_t> Globals::GetBaseromFile(std::string fileName)
{
	if (fileMode == ZFileMode::ExtractDirectory)
	{
		if (StringHelper::Contains(fileName, "baserom/"))
			fileName = StringHelper::Split(fileName, "baserom/")[1];

		return rom->GetFile(fileName);

	}
	else
		return DiskFile::ReadAllBytes(fileName);
}

bool Globals::GetSegmentedPtrName(segptr_t segAddress, ZFile* currentFile,
                                  const std::string& expectedType, std::string& declName,
                                  int workerID, bool warnIfNotFound)
{
	if (segAddress == SEGMENTED_NULL)
	{
		declName = "NULL";
		return true;
	}

	uint8_t segment = GETSEGNUM(segAddress);
	uint32_t offset = Seg2Filespace(segAddress, currentFile->baseAddress);
	ZSymbol* sym;

	sym = currentFile->GetSymbolResource(offset);
	if (sym != nullptr)
	{
		if (expectedType == "" || expectedType == sym->GetSourceTypeName())
		{
			declName = sym->GetName();
			return true;
		}
	}
	sym = currentFile->GetSymbolResource(segAddress);
	if (sym != nullptr)
	{
		if (expectedType == "" || expectedType == sym->GetSourceTypeName())
		{
			declName = sym->GetName();
			return true;
		}
	}

	if (currentFile->IsSegmentedInFilespaceRange(segAddress))
	{
		if (currentFile->GetDeclarationPtrName(segAddress, expectedType, declName))
			return true;
	}
	else if (HasSegment(segment, workerID))
	{
		// OTRTODO: Multithreading
		auto segs = GetSegmentRefFiles(workerID);
		for (auto file : segs[segment])
		{
			offset = Seg2Filespace(segAddress, file->baseAddress);

			sym = file->GetSymbolResource(offset);
			if (sym != nullptr)
			{
				if (expectedType == "" || expectedType == sym->GetSourceTypeName())
				{
					declName = sym->GetName();
					return true;
				}
			}
			sym = file->GetSymbolResource(segAddress);
			if (sym != nullptr)
			{
				if (expectedType == "" || expectedType == sym->GetSourceTypeName())
				{
					declName = sym->GetName();
					return true;
				}
			}

			if (file->IsSegmentedInFilespaceRange(segAddress))
			{
				if (file->GetDeclarationPtrName(segAddress, expectedType, declName))
					return true;
			}
		}
	}

	const auto& symbolFromMap = Globals::Instance->cfg.symbolMap.find(segAddress);
	if (symbolFromMap != Globals::Instance->cfg.symbolMap.end())
	{
		declName = "&" + symbolFromMap->second;
		return true;
	}

	declName = StringHelper::Sprintf("0x%08X", segAddress);
	if (warnIfNotFound)
	{
		WarnHardcodedPointer(segAddress, currentFile, nullptr, -1);
	}
	return false;
}

bool Globals::GetSegmentedArrayIndexedName(segptr_t segAddress, size_t elementSize,
                                           ZFile* currentFile, const std::string& expectedType,
                                           std::string& declName, int workerID, bool warnIfNotFound)
{
	if (segAddress == SEGMENTED_NULL)
	{
		declName = "NULL";
		return true;
	}

	uint8_t segment = GETSEGNUM(segAddress);

	if (currentFile->IsSegmentedInFilespaceRange(segAddress))
	{
		bool addressFound = currentFile->GetDeclarationArrayIndexedName(segAddress, elementSize,
		                                                                expectedType, declName);
		if (addressFound)
			return true;
	}
	else if (HasSegment(segment, workerID))
	{
		// OTRTODO: Multithreading
		auto segs = GetSegmentRefFiles(workerID);
		for (auto file : segs[segment])
		{
			if (file->IsSegmentedInFilespaceRange(segAddress))
			{
				bool addressFound = file->GetDeclarationArrayIndexedName(segAddress, elementSize,
				                                                         expectedType, declName);
				if (addressFound)
					return true;
			}
		}
	}

	declName = StringHelper::Sprintf("0x%08X", segAddress);
	if (warnIfNotFound)
	{
		WarnHardcodedPointer(segAddress, currentFile, nullptr, -1);
	}
	return false;
}

void Globals::WarnHardcodedPointer(segptr_t segAddress, ZFile* currentFile, ZResource* res,
                                   offset_t currentOffset)
{
	uint8_t segment = GETSEGNUM(segAddress);

	if ((segment >= 2 && segment <= 6) || segment == 0x80)
	{
		std::string errorHeader = "A hardcoded pointer was found";
		std::string errorBody = StringHelper::Sprintf("Pointer: 0x%08X", segAddress);

		HANDLE_WARNING_RESOURCE(WarningType::HardcodedPointer, currentFile, res, currentOffset,
		                        errorHeader, errorBody);
	}
	else
	{
		std::string errorHeader = "A general purpose hardcoded pointer was found";
		std::string errorBody = StringHelper::Sprintf("Pointer: 0x%08X", segAddress);

		HANDLE_WARNING_RESOURCE(WarningType::HardcodedGenericPointer, currentFile, res,
		                        currentOffset, errorHeader, errorBody);
	}
}

ExternalFile::ExternalFile(fs::path nXmlPath, fs::path nOutPath)
	: xmlPath{nXmlPath}, outPath{nOutPath}
{
}

ExporterSet::~ExporterSet()
{
	for (auto& it : exporters)
	{
		delete it.second;
	}
}
