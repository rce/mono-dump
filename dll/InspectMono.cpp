#include "InspectMono.h"

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <locale>
#include <codecvt>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <set>

#include "../dll-common/mono.h"
#include <mono/metadata/attrdefs.h>

#define PRINT_HEX(x) std::cout << #x << " " << std::hex << (x) << std::dec << std::endl
#define LOG(x) std::cerr << #x << " == " << x << std::endl

struct Field {
	std::string type_name;
	int type_size;
	std::string field_name;
	uint32_t offset;
	unsigned int field_flags;

	bool IsStatic() { return field_flags & MONO_FIELD_ATTR_STATIC; }
};

void CleanupIdentifier(std::string& s) {
	std::vector<char> invalid_chars = { '<', '>', ' ', '`', '=', '[', ']', '(', ')', ',', '.', ':', ';', '\'', '\"', '/', '\\', '\t', '\r', '\n' };
	s.erase(std::remove_if(s.begin(), s.end(), [invalid_chars](char c) {
		return std::find(invalid_chars.begin(), invalid_chars.end(), c) != invalid_chars.end();
	}), s.end());
	//s.erase(std::remove(s.begin(), s.end(), invalid_chars));
}

std::vector<Field> get_fields(MonoClass* klass) {
	std::vector<Field> fields;
	void* iter = nullptr;
	MonoClassField* field = nullptr;
	do {
		field = mono::class_get_fields(klass, &iter);
		if (field) {
			auto field_name = mono::field_get_name(field);
			auto field_type = mono::field_get_type(field);
			auto field_type_name = mono::type_get_name(field_type);
			auto field_flags = mono::field_get_flags(field);
			auto field_offset = mono::field_get_offset(field);
			int align;
			auto field_type_size = mono::type_size(field_type, &align);
			fields.push_back(Field{ field_type_name, field_type_size, field_name, field_offset, field_flags });
		}
	} while (field);
	std::sort(fields.begin(), fields.end(), [](Field a, Field b) { return a.offset < b.offset; });
	return fields;
}

std::string replace_all(std::string src, std::string old, std::string replacement) {
	size_t pos = 0;
	while ((pos = src.find(old, pos)) != std::string::npos) {
		src.replace(pos, old.length(), replacement);
		pos += replacement.length();
	}
	return src;
}


void DumpMono() {
	mono::ThreadAttachment thread{};
	std::ofstream output("monodump.h");

	for (auto& assembly : mono::get_assemblies()) {
		auto image = mono::assembly_get_image(assembly);
		auto image_name = std::string(mono::image_get_name(image));
		auto tdef = mono::image_get_table_info(image, MONO_TABLE_TYPEDEF);
		if (!tdef) continue;
		auto tdefcount = mono::table_info_get_rows(tdef);
		for (int i = 0; i < tdefcount; i++)
		{
			auto klass = mono::class_get(image, MONO_TOKEN_TYPE_DEF | (i + 1));
			auto name = std::string(mono::class_get_name(klass));
			auto ns = std::string(mono::class_get_namespace(klass));
			if (!ns.starts_with("CrusadersGame.GameScreen")) continue;
			CleanupIdentifier(name);
			output << "struct " << name << "; \n";
		}
		for (int i = 0; i < tdefcount; i++)
		{
			auto klass = mono::class_get(image, MONO_TOKEN_TYPE_DEF | (i + 1));
			auto name = std::string(mono::class_get_name(klass));
			auto ns = std::string(mono::class_get_namespace(klass));

			if (!ns.starts_with("CrusadersGame.GameScreen")) continue;
			if (name == "ParalaxBackground") continue;
			//std::cout << ns << "." << name << "\n";
			output << "// " << ns << "." << name << "\n";
			CleanupIdentifier(name);
			output << "struct " << name << " {\n";
			output << "  /*MonoVTable*/ void* vtable;\n";
			output << "  /*MonoThreadsSync*/ void* synchronisation;\n";
			for (auto& f : get_fields(klass)) {
				std::string cpp_type;
				auto simple_name = f.type_name.substr(f.type_name.rfind(".") + 1);
				std::set<std::string> known_types = { "ActiveCampaignData", "CrusadersGameController", "Area", "AreaLevel" };
				if (f.type_name == "System.Int32") {
					cpp_type = "int32_t";
				}
				else if (known_types.contains(simple_name)) {
					// todo parse any known type
					//cpp_type = replace_all(f.type_name, ".", "::") + "*";
					cpp_type = f.type_name + "*";
					cpp_type = cpp_type.substr(cpp_type.rfind(".") + 1);
				}
				else if (f.type_name == "CrusadersGame.GameScreen.ActiveCampaignData") {
					cpp_type = "ActiveCampaignData*";
				} else {
					cpp_type = f.type_size == sizeof(void*) ? "void*"
						: f.type_size == sizeof(uint32_t) ? "uint32_t"
						: f.type_size == sizeof(uint16_t) ? "uint16_t"
						: f.type_size == sizeof(uint8_t) ? "uint8_t"
						: "void*";
				}
				if (f.IsStatic()) output << "  // static ";
				else output << "  ";

				CleanupIdentifier(f.field_name);
				f.field_name.erase(std::remove_if(f.field_name.begin(), f.field_name.end(), [](char c) { return c == '<' || c == '>'; }));
				output << "/*" << f.type_name << "*/ " << cpp_type << " " << f.field_name << "; // Offset: 0x" << std::hex << f.offset << "\n";
			}
			output << "};\n\n";
		}
	}
	output.close();
};


void InspectMono() {
	// TODO Sometimes the module is mono.dll, sometimes mono-2.0-bdwgc. One could use something like
	// CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId()) and GetProccAddress("mono_thread_attach")
	// or something to figure out the proper module.
	HMODULE hModule = GetModuleHandle(mono::ModuleName);

	if (!hModule) {
		std::cout << "Mono DLL not found" << std::endl;
		return;
	}

	if (!GetProcAddress(hModule, "mono_thread_attach")) {
		std::cout << "mono_thread_attach not found" << std::endl;
		return;
	}

	std::cout << "Mono DLL location: " << std::hex << hModule << std::dec << std::endl;
	if (GetProcAddress(hModule, "il2cpp_thread_attach")) {
		std::cout << "Using il2cpp, no can do" << std::endl;
		return;
	}
	DumpMono();
}
