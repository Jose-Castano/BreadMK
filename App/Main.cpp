#include <Collections/String.hpp>
#include <Collections/StringView.hpp>
#include <Collections/StringMap.hpp>
#include <Debug/Log.hpp>
#include <fmt/fmt.h>
#include <OS/OS.hpp>
#include <Runtime/Runtime.hpp>


struct Option
{
    void(*command)(Mem::Allocator& allocator, Slice<Collections::StringView> arguments);
};

// options
void command_help(Mem::Allocator&, Slice<Collections::StringView>)
{
    Log::info(
        "Bread Maker:\n"
        "\thelp: Shows this help.\n"
        "\tnew: Creates a new project."
    );
}

void create_project(Mem::Allocator& allocator, Collections::StringView output_directory,
    Collections::StringView name)
{
    Collections::ResultOnly<OS::CreateDirectoryError> result = OS::create_directories(allocator, output_directory);
    if(result.fail() && result.error() != OS::CreateDirectoryError::AlreadyExists)
    {
        Log::info("Fail to create a new project");
        return;
    }

    { // .bmk
        Collections::String project_file_path{allocator, 0, output_directory};
        project_file_path.add("/");
        project_file_path.add(name);
        project_file_path.add(".bmk");
        
        IO::File project_file{allocator, project_file_path.view(), IO::File::OpenMode::Create | IO::File::OpenMode::Write};
        Format::format<true>(project_file.writer(),
            "project \"{}\"\n"
            "{{\n"
            "\tsources = [\"src/Main.cpp\"]\n"
            "\tincludes = [\"src/\"]\n"
            "}"
            ,
            name
        );
    }

    { // sub folders
        Collections::String source_folder_path{allocator, 0, output_directory};
        source_folder_path.add("/src/");

        result = OS::create_directories(allocator, source_folder_path.view());
        if(result.fail() && result.error() != OS::CreateDirectoryError::AlreadyExists)
        {
            Log::info("Error: Fail to create a source folder");
            return;
        }

        Collections::String source_main_path{allocator, 0, output_directory};
        source_main_path.add("/src/Main.cpp");

        IO::File main_source_file{allocator, source_main_path.view(), IO::File::OpenMode::Create | IO::File::OpenMode::Write};
        Format::format<true>(main_source_file.writer(),
            "#include <Runtime/Runtime.hpp>\n"
            "#include <Debug/Log.hpp>\n"
            "\n"
            "\n"
            "void BreadMain(const Runtime::ProcessInit&)\n"
            "{{\n"
            "    Log::debug(\"Hello, {{}\", \"World\");\n"
            "}"
        );
    }
}

void command_new(Mem::Allocator& allocator, Slice<Collections::StringView> arguments)
{
    if(arguments.len == 0)
    {
        Log::info(
            "BreadMK new: name [args...]\n"
            "args:\n"
            "\t-o Output directory"
        );
        return;
    }

    Collections::StringView name = arguments[0];

    Slice<Collections::StringView> remainder = arguments.add(1);
    Collections::String output_directory{allocator, 0, name};

    for(usize i = 0; i < remainder.len; i++)
    {
        Collections::StringView arg = remainder[i];
        if(arg[0] == '-')
        {
            Collections::StringView flag = arg.add(1);
            if(flag.equals("o"))
            {
                // try consume next argument
                if(i + 1 < remainder.len)
                {
                    Collections::StringView next_arg = remainder[i + 1];
                    output_directory.set(next_arg);
                    i++;
                }
                else
                {
                    Log::info("Error: a output folder was expected");
                }
            }
        }
    }

    create_project(allocator, output_directory.view(), name);
}

// entry point

void invalid_args()
{
    Log::info("Error: use the command 'help' for more info");
}

void BreadMain(const Runtime::ProcessInit& process_info)
{
    // command in first argument
    if(process_info.arguments.len == 1)
    {
        invalid_args();
        return;
    }

    Collections::StringView option = process_info.arguments[1];

    // [...].exe "option" args...
    Slice<Collections::StringView> remain = process_info.arguments.add(2);

    // available options
    Collections::StringMap<Option> options{process_info.allocator, 4};
    (void)options.insert("help", Option{.command = &command_help});
    (void)options.insert("new", Option{.command = &command_new});

    if(!options.has(option))
    {
        invalid_args();
    }
    else
    {
        options.get(option).command(process_info.allocator, remain);
    }
}
