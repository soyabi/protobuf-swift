// Protocol Buffers for Swift
//
// Copyright 2014 Alexey Khohklov(AlexeyXo).
// Copyright 2008 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "swift_helpers.h"

#include <limits>
#include <vector>

#include <google/protobuf/stubs/hash.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

#include "google/protobuf/swift-descriptor.pb.h"

namespace google { namespace protobuf { namespace compiler { namespace swift {
    namespace {
        const string& FieldName(const FieldDescriptor* field) {
            if (field->type() == FieldDescriptor::TYPE_GROUP) {
                return field->message_type()->name();
            } else {
                return field->name();
            }
        }
    }
    
    string CheckReservedNames(const string& input)
    {
        string result;
        if (input == "extension" || input == "description") {
            result = input + "_";
        }
        else if (input == "Type") {
            result = input + "s";
        }
        else
        {
            result = input;
        }
        return result;
    }
    
    
    string UnderscoresToCapitalizedCamelCase(const string& input) {
        vector<string> values;
        string current;
        
        bool last_char_was_number = false;
        bool last_char_was_lower = false;
        bool last_char_was_upper = false;
        for (unsigned int i = 0; i < input.size(); i++) {
            char c = input[i];
            if (c >= '0' && c <= '9') {
                if (!last_char_was_number) {
                    values.push_back(current);
                    current = "";
                }
                current += c;
                last_char_was_number = last_char_was_lower = last_char_was_upper = false;
                last_char_was_number = true;
            } else if (c >= 'a' && c <= 'z') {
                // lowercase letter can follow a lowercase or uppercase letter
                if (!last_char_was_lower && !last_char_was_upper) {
                    values.push_back(current);
                    current = "";
                }
                current += c;
                last_char_was_number = last_char_was_lower = last_char_was_upper = false;
                last_char_was_lower = true;
            } else if (c >= 'A' && c <= 'Z') {
                if (!last_char_was_upper) {
                    values.push_back(current);
                    current = "";
                }
                current += c;
                last_char_was_number = last_char_was_lower = last_char_was_upper = false;
                last_char_was_upper = true;
            } else {
                last_char_was_number = last_char_was_lower = last_char_was_upper = false;
            }
        }
        values.push_back(current);
        
        for (vector<string>::iterator i = values.begin(); i != values.end(); ++i) {
            string value = *i;
            for (unsigned int j = 0; j < value.length(); j++) {
                if (j == 0) {
                    value[j] = toupper(value[j]);
                } else {
                    value[j] = tolower(value[j]);
                }
            }
            *i = value;
        }
        string result;
        for (vector<string>::iterator i = values.begin(); i != values.end(); ++i) {
            result += *i;
        }
        return CheckReservedNames(result);
    }
    
    
    string UnderscoresToCamelCase(const string& input) {
        string result = UnderscoresToCapitalizedCamelCase(input);
        
        if (result.length() == 0) {
            return result;
        }
        
        result[0] = tolower(result[0]);
        return CheckReservedNames(result);
    }
    
    
    string UnderscoresToCamelCase(const FieldDescriptor* field) {
        return UnderscoresToCamelCase(FieldName(field));
    }
    
    
    string UnderscoresToCapitalizedCamelCase(const FieldDescriptor* field) {
        return UnderscoresToCapitalizedCamelCase(FieldName(field));
    }
    
    
    string UnderscoresToCamelCase(const MethodDescriptor* method) {
        return UnderscoresToCamelCase(method->name());
    }
    
    
    string FilenameToCamelCase(const string& filename) {
        string result;
        bool need_uppercase = true;
        
        result.reserve(filename.length());
        
        for (string::const_iterator it(filename.begin()), itEnd(filename.end()); it != itEnd; ++it) {
            const char c = *it;
            
            // Ignore undesirable characters.  The good character must be
            // uppercased, though.
            if (!isalnum(c) && c != '_') {
                need_uppercase = true;
                continue;
            }
            
            // If an uppercased character has been requested, transform the current
            // character, append it to the result, reset the flag, and move on.
            // This is safe to do even if the character is already uppercased.
            if (need_uppercase && isalpha(c)) {
                result += toupper(c);
                need_uppercase = false;
                continue;
            }
            
            // Simply append this character.
            result += c;
            
            // If this character was a digit, we want the next character to be an
            // uppercased letter.
            if (isdigit(c)) {
                need_uppercase = true;
            }
        }
        
        return result;
    }
    
    
    string StripProto(const string& filename) {
        if (HasSuffixString(filename, ".protodevel")) {
            return StripSuffixString(filename, ".protodevel");
        } else {
            return StripSuffixString(filename, ".proto");
        }
    }
    
    bool IsBootstrapFile(const FileDescriptor* file) {
        return file->name() == "google/protobuf/descriptor.proto";
    }
    
    
    string FileName(const FileDescriptor* file) {
        string basename;
        
        string::size_type last_slash = file->name().find_last_of('/');
        if (last_slash == string::npos) {
            basename += file->name();
        } else {
            basename += file->name().substr(last_slash + 1);
        }
        
        return FilenameToCamelCase(StripProto(basename));
    }
    
    
    string FilePath(const FileDescriptor* file) {
        string path = FileName(file);
        
        if (file->options().HasExtension(swift_file_options)) {
            SwiftFileOptions options = file->options().GetExtension(swift_file_options);
            
            if (options.package() != "") {
                path = options.package() + "/" + path;
            }
        }
        
        return path;
    }
    
    
    string FileClassPrefix(const FileDescriptor* file) {
        if (IsBootstrapFile(file)) {
            return "PB";
        } else if (file->options().HasExtension(swift_file_options)) {
            SwiftFileOptions options = file->options().GetExtension(swift_file_options);
            
            return options.class_prefix();
        } else {
            return "";
        }
    }
    
    
    string FileClassName(const FileDescriptor* file) {
        // Ensure the FileClassName is camelcased irrespective of whether the
        // camelcase_output_filename option is set.
        return FileClassPrefix(file) +
        UnderscoresToCapitalizedCamelCase(FileName(file)) + "Root";
    }
    
    
    string ToSwiftName(const string& full_name, const FileDescriptor* file) {
        string result;
        result += FileClassPrefix(file);
        result += full_name;
        return result;
    }
    
    
    string ClassNameWorker(const Descriptor* descriptor) {
        string name;
        if (descriptor->containing_type() != NULL) {
            name = ClassNameWorker(descriptor->containing_type());
            name += ".";
        }
        return CheckReservedNames(name + descriptor->name());
    }
    
    string ClassNameWorkerExtensions(const Descriptor* descriptor) {
        string name;
        if (descriptor->containing_type() != NULL) {
            name = ClassNameWorker(descriptor->containing_type());
            name += "";
        }
        return CheckReservedNames(name + descriptor->name());
    }
    
    
    string ClassNameWorker(const EnumDescriptor* descriptor) {
        string name;
        if (descriptor->containing_type() != NULL) {
            name = ClassNameWorker(descriptor->containing_type());
            name += ".";
        }
        return CheckReservedNames(name + UnderscoresToCapitalizedCamelCase(descriptor->name()));
    }
    
    
    string ClassName(const Descriptor* descriptor) {
        string name;
        name += FileClassPrefix(descriptor->file());
        name += ClassNameWorker(descriptor);
        return CheckReservedNames(name);
    }
    string ClassNameExtensions(const Descriptor* descriptor) {
        string name;
        name += FileClassPrefix(descriptor->file());
        name += ClassNameWorkerExtensions(descriptor);
        return CheckReservedNames(name);
    }
    
    string ClassNameMessage(const Descriptor* descriptor) {
        string name;
        name += FileClassPrefix(descriptor->file());
        if (descriptor->containing_type() != NULL) {
            
            return CheckReservedNames(UnderscoresToCapitalizedCamelCase(descriptor->name()));
        }
        else
        {
            name += descriptor->name();
        }
        return CheckReservedNames(name);
    }
    
    
    string ClassNameOneof(const OneofDescriptor* descriptor) {
        string name;
        if (descriptor->containing_type() != NULL) {
            name = ClassNameWorker(descriptor->containing_type());
            name += ".";
        }
        return CheckReservedNames(name + UnderscoresToCapitalizedCamelCase(descriptor->name()));
    }
    
    bool isOneOfField(const FieldDescriptor* descriptor) {
        if (descriptor->containing_oneof())
        {
            return true;
            
        }
        return false;
    }
    
    
    string ClassName(const EnumDescriptor* descriptor) {
        string name;
        name += FileClassPrefix(descriptor->file());
        name += ClassNameWorker(descriptor);
        return CheckReservedNames(name);
    }
    
    
    string ClassName(const ServiceDescriptor* descriptor) {
        string name;
        name += FileClassPrefix(descriptor->file());
        name += descriptor->name();
        return CheckReservedNames(name);
    }
    
    //Swift bug: when enumName == enaumFieldName
    string EnumValueName(const EnumValueDescriptor* descriptor) {
        string name = UnderscoresToCapitalizedCamelCase(SafeName(descriptor->name()));
        if (name == UnderscoresToCapitalizedCamelCase(descriptor->type()->name())) {
            name += "Field";
        }
        return name;
    }
    
    
    SwiftType GetSwiftType(FieldDescriptor::Type field_type) {
        switch (field_type) {
            case FieldDescriptor::TYPE_INT32:
            case FieldDescriptor::TYPE_UINT32:
            case FieldDescriptor::TYPE_SINT32:
            case FieldDescriptor::TYPE_FIXED32:
            case FieldDescriptor::TYPE_SFIXED32:
                return SWIFTTYPE_INT;
                
            case FieldDescriptor::TYPE_INT64:
            case FieldDescriptor::TYPE_UINT64:
            case FieldDescriptor::TYPE_SINT64:
            case FieldDescriptor::TYPE_FIXED64:
            case FieldDescriptor::TYPE_SFIXED64:
                return SWIFTTYPE_LONG;
                
            case FieldDescriptor::TYPE_FLOAT:
                return SWIFTTYPE_FLOAT;
                
            case FieldDescriptor::TYPE_DOUBLE:
                return SWIFTTYPE_DOUBLE;
                
            case FieldDescriptor::TYPE_BOOL:
                return SWIFTTYPE_BOOLEAN;
                
            case FieldDescriptor::TYPE_STRING:
                return SWIFTTYPE_STRING;
                
            case FieldDescriptor::TYPE_BYTES:
                return SWIFTTYPE_DATA;
                
            case FieldDescriptor::TYPE_ENUM:
                return SWIFTTYPE_ENUM;
                //
                //    case FieldDescriptor::TYPE_ONEOF:
                //        return SWIFTTYPE_ONEOF;
                
            case FieldDescriptor::TYPE_GROUP:
            case FieldDescriptor::TYPE_MESSAGE:
                return SWIFTTYPE_MESSAGE;
        }
        
        GOOGLE_LOG(FATAL) << "Can't get here.";
        return SWIFTTYPE_INT;
    }
    
    
    const char* BoxedPrimitiveTypeName(SwiftType type) {
        switch (type) {
            case SWIFTTYPE_INT    : return "Int32";
            case SWIFTTYPE_LONG   : return "Int64";
            case SWIFTTYPE_FLOAT  : return "Float";
            case SWIFTTYPE_DOUBLE : return "Double";
            case SWIFTTYPE_BOOLEAN: return "Bool";
            case SWIFTTYPE_STRING : return "String";
            case SWIFTTYPE_DATA   : return "[Byte]";
            case SWIFTTYPE_ENUM   : return "Int32";
            case SWIFTTYPE_MESSAGE: return NULL;
        }
        
        GOOGLE_LOG(FATAL) << "Can't get here.";
        return NULL;
    }
    
    
    bool IsPrimitiveType(SwiftType type) {
        switch (type) {
            case SWIFTTYPE_INT    :
            case SWIFTTYPE_LONG   :
            case SWIFTTYPE_FLOAT  :
            case SWIFTTYPE_DOUBLE :
            case SWIFTTYPE_BOOLEAN:
            case SWIFTTYPE_ENUM   :
                return true;
                
            default:
                return false;
        }
    }
    
    
    bool IsReferenceType(SwiftType type) {
        return !IsPrimitiveType(type);
    }
    
    
    bool ReturnsPrimitiveType(const FieldDescriptor* field) {
        return IsPrimitiveType(GetSwiftType(field->type()));
    }
    
    
    bool ReturnsReferenceType(const FieldDescriptor* field) {
        return !ReturnsPrimitiveType(field);
    }
    
    
    namespace {
        string DotsToUnderscores(const string& name) {
            return StringReplace(name, ".", "_", true);
        }
        
        const char* const kKeywordList[] = {
            "TYPE_BOOL"
        };
        
        
        hash_set<string> MakeKeywordsMap() {
            hash_set<string> result;
            for (unsigned int i = 0; i < GOOGLE_ARRAYSIZE(kKeywordList); i++) {
                result.insert(kKeywordList[i]);
            }
            return result;
        }
        
        hash_set<string> kKeywords = MakeKeywordsMap();
    }
    
    
    string SafeName(const string& name) {
        string result = name;
        if (kKeywords.count(result) > 0) {
            result.append("_");
        }
        return result;
    }
    
    bool AllAscii(const string& text) {
        for (unsigned int i = 0; i < text.size(); i++) {
            if ((text[i] & 0x80) != 0) {
                return false;
            }
        }
        return true;
    }
    
    string DefaultValue(const FieldDescriptor* field) {
        // Switch on cpp_type since we need to know which default_value_* method
        // of FieldDescriptor to call.
        switch (field->cpp_type()) {
            case FieldDescriptor::CPPTYPE_INT32:  return "Int32(" + SimpleItoa(field->default_value_int32()) +")";
            case FieldDescriptor::CPPTYPE_UINT32: return "UInt32(" + SimpleItoa(field->default_value_uint32()) + ")";
            case FieldDescriptor::CPPTYPE_INT64:  return "Int64(" + SimpleItoa(field->default_value_int64()) + ")";
            case FieldDescriptor::CPPTYPE_UINT64: return "UInt64(" + SimpleItoa(field->default_value_uint64()) + ")";
            case FieldDescriptor::CPPTYPE_BOOL:   return field->default_value_bool() ? "true" : "false";
            case FieldDescriptor::CPPTYPE_DOUBLE: {
                const double value = field->default_value_double();
                if (value == numeric_limits<double>::infinity()) {
                    return "Double(HUGE)";
                } else if (value == -numeric_limits<double>::infinity()) {
                    return "Double(-HUGE)";
                } else if (value != value) {
                    return "0.0";
                } else {
                    return "Double("+ SimpleDtoa(field->default_value_double()) + ")";
                }
            }
            case FieldDescriptor::CPPTYPE_FLOAT: {
                const float value = field->default_value_float();
                if (value == numeric_limits<float>::infinity()) {
                    return "HUGE";
                } else if (value == -numeric_limits<float>::infinity()) {
                    return "-HUGE";
                } else if (value != value) {
                    return "0.0";
                } else {
                    return "Float(" + SimpleFtoa(value) + ")";
                }
            }
            case FieldDescriptor::CPPTYPE_STRING:
                if (field->type() == FieldDescriptor::TYPE_BYTES) {
                    if (field->has_default_value()) {
                        return
                        "([Byte]() + \"" + CEscape(field->default_value_string()) + "\".utf8)";
                    } else {
                        return "[Byte]()";
                    }
                } else {
                    if (AllAscii(field->default_value_string())) {
                        return "\"" + EscapeTrigraphs(CEscape(EscapeUnicode(field->default_value_string()))) + "\"";
                    } else {
                        return "\"" + EscapeTrigraphs(CEscape(EscapeUnicode(field->default_value_string()))) + "\"";
                    }
                }
            case FieldDescriptor::CPPTYPE_ENUM:
                return EnumValueName(field->default_value_enum());
            case FieldDescriptor::CPPTYPE_MESSAGE:
                return ClassName(field->message_type()) + "()";
        }
        
        GOOGLE_LOG(FATAL) << "Can't get here.";
        return "";
    }
    
    //  const char* GetArrayValueType(const FieldDescriptor* field) {
    //    switch (field->type()) {
    //      case FieldDescriptor::TYPE_INT32   : return "Int32" ;
    //      case FieldDescriptor::TYPE_UINT32  : return "UInt32";
    //      case FieldDescriptor::TYPE_SINT32  : return "Int32" ;
    //      case FieldDescriptor::TYPE_FIXED32 : return "UInt32";
    //      case FieldDescriptor::TYPE_SFIXED32: return "Int32" ;
    //      case FieldDescriptor::TYPE_INT64   : return "Int64" ;
    //      case FieldDescriptor::TYPE_UINT64  : return "UInt64";
    //      case FieldDescriptor::TYPE_SINT64  : return "Int64" ;
    //      case FieldDescriptor::TYPE_FIXED64 : return "UInt64";
    //      case FieldDescriptor::TYPE_SFIXED64: return "PBArrayValueTypeInt64" ;
    //      case FieldDescriptor::TYPE_FLOAT   : return "PBArrayValueTypeFloat" ;
    //      case FieldDescriptor::TYPE_DOUBLE  : return "PBArrayValueTypeDouble";
    //      case FieldDescriptor::TYPE_BOOL    : return "PBArrayValueTypeBool"  ;
    //      case FieldDescriptor::TYPE_STRING  : return "PBArrayValueTypeObject";
    //      case FieldDescriptor::TYPE_BYTES   : return "PBArrayValueTypeObject";
    //      case FieldDescriptor::TYPE_ENUM    : return "PBArrayValueTypeObject";
    //      case FieldDescriptor::TYPE_GROUP   : return "PBArrayValueTypeObject";
    //      case FieldDescriptor::TYPE_MESSAGE : return "PBArrayValueTypeObject";
    //    }
    //
    //    GOOGLE_LOG(FATAL) << "Can't get here.";
    //    return NULL;
    //  }
    
    // Escape C++ trigraphs by escaping question marks to \?
    
    string EscapeTrigraphs(const string& to_escape) {
        return StringReplace(to_escape, "?", "\\?", true);
    }
    
    string EscapeUnicode(const string& to_escape) {
        return to_escape;//StringReplace(to_escape, "\", "", true);
        
    }
    
}  // namespace swift
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
