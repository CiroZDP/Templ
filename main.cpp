#include <iostream>
#include <ncurses.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>

#define KEY_SHIFT 0x146

void initializeEditor() {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    start_color();
}

void closeEditor() {
    endwin();
}

void editTemplate(const std::string& templateName) {
    initializeEditor();
    
    std::vector<std::filesystem::path> dirTree;
    std::vector<std::filesystem::path> currentDirContents;
    std::vector<std::string> fileBuffer;
    
    int currentSection = 0; // 0 = main tree, 1 = subdirectory view, 2 = file edit
    int selectedItem = 0;
    int subSelectedItem = 0;
    std::filesystem::path templateDir = std::filesystem::path("../templates") / templateName;
    std::filesystem::path currentPath = templateDir;
    int cursorX = 0, cursorY = 0;
    std::string clipboard = "";
    bool selectionMode = false;
    int selStartX = 0, selStartY = 0;
    bool modified = false;
    
    if (!std::filesystem::exists(templateDir)) {
        closeEditor();
        std::cout << "Template not found: " << templateName << std::endl;
        return;
    }
    
    auto updateDirTree = [&]() {
        dirTree.clear();
        for (const auto& entry : std::filesystem::directory_iterator(currentPath)) {
            dirTree.push_back(entry.path());
        }
    };
    
    auto updateSubDir = [&]() {
        currentDirContents.clear();
        if (selectedItem < dirTree.size() && std::filesystem::is_directory(dirTree[selectedItem])) {
            for (const auto& entry : std::filesystem::directory_iterator(dirTree[selectedItem])) {
                currentDirContents.push_back(entry.path());
            }
        }
    };
    
    auto saveFile = [&]() {
        if (currentSection == 2 && subSelectedItem < currentDirContents.size()) {
            std::ofstream file(currentDirContents[subSelectedItem]);
            for (const auto& line : fileBuffer) {
                file << line << std::endl;
            }
            modified = false;
        }
    };
    
    updateDirTree();
    
    while (true) {
        clear();
        
        if (currentSection == 2) {
            // File editing mode - use full screen
            for (size_t i = 0; i < fileBuffer.size(); i++) {
                mvprintw(i, 0, "%s", fileBuffer[i].c_str());
            }
            
            // Show cursor
            move(cursorY, cursorX);
            
            // Show selection if active
            if (selectionMode) {
                int startY = std::min(selStartY, cursorY);
                int endY = std::max(selStartY, cursorY);
                int startX = std::min(selStartX, cursorX);
                int endX = std::max(selStartX, cursorX);
                
                for (int y = startY; y <= endY; y++) {
                    attron(A_REVERSE);
                    if (y < fileBuffer.size()) {
                        int lineLen = fileBuffer[y].length();
                        mvprintw(y, startX, "%s", fileBuffer[y].substr(startX, endX - startX).c_str());
                    }
                    attroff(A_REVERSE);
                }
            }
        } else {
            // Directory view
            for (int i = 0; i < LINES; i++) {
                mvaddch(i, 30, ACS_VLINE);
                if (currentSection > 0) mvaddch(i, 60, ACS_VLINE);
            }
            
            mvprintw(0, 2, "Template: %s", templateName.c_str());
            for (size_t i = 0; i < dirTree.size(); i++) {
                if (i == selectedItem && currentSection == 0) {
                    attron(A_REVERSE);
                }
                mvprintw(i + 2, 2, "%s", dirTree[i].filename().string().c_str());
                if (i == selectedItem && currentSection == 0) {
                    attroff(A_REVERSE);
                }
            }
            
            if (currentSection >= 1 && selectedItem < dirTree.size()) {
                mvprintw(0, 32, "Contents of: %s", dirTree[selectedItem].filename().string().c_str());
                for (size_t i = 0; i < currentDirContents.size(); i++) {
                    if (i == subSelectedItem && currentSection == 1) {
                        attron(A_REVERSE);
                    }
                    mvprintw(i + 2, 32, "%s", currentDirContents[i].filename().string().c_str());
                    if (i == subSelectedItem && currentSection == 1) {
                        attroff(A_REVERSE);
                    }
                }
            }
        }
        
        refresh();
        
        int ch = getch();
        
        if (currentSection == 2) {
            if (ch == 27) { // ESC
                if (modified) {
                    mvprintw(LINES-1, 0, "Save changes? (y/n): ");
                    refresh();
                    int response = getch();
                    if (response == 'y' || response == 'Y') {
                        saveFile();
                    }
                }
                currentSection = 1;
                continue;
            }
            
            if (ch == 19) { // Ctrl-S
                saveFile();
                currentSection = 1;
                continue;
            }
            
            if (ch == KEY_UP && cursorY > 0) cursorY--;
            if (ch == KEY_DOWN && cursorY < fileBuffer.size()) cursorY++;
            if (ch == KEY_LEFT && cursorX > 0) cursorX--;
            if (ch == KEY_RIGHT) {
                if (cursorY < fileBuffer.size()) {
                    if (cursorX < fileBuffer[cursorY].length()) {
                        cursorX++;
                    } else {
                        cursorX = fileBuffer[cursorY].length();
                    }
                }
            }
            
            if (ch == KEY_SHIFT) {
                selectionMode = !selectionMode;
                if (selectionMode) {
                    selStartX = cursorX;
                    selStartY = cursorY;
                }
            }
            
            if (ch == 3) { // Ctrl-C
                if (selectionMode) {
                    clipboard = "";
                    int startY = std::min(selStartY, cursorY);
                    int endY = std::max(selStartY, cursorY);
                    int startX = std::min(selStartX, cursorX);
                    int endX = std::max(selStartX, cursorX);
                    
                    for (int y = startY; y <= endY; y++) {
                        if (y < fileBuffer.size()) {
                            clipboard += fileBuffer[y].substr(startX, endX - startX);
                            if (y != endY) clipboard += "\n";
                        }
                    }
                }
            }
            
            if (ch == 22) { // Ctrl-V
                if (!clipboard.empty()) {
                    std::string line;
                    std::istringstream stream(clipboard);
                    while (std::getline(stream, line)) {
                        if (cursorY >= fileBuffer.size()) {
                            fileBuffer.push_back("");
                        }
                        fileBuffer[cursorY].insert(cursorX, line);
                        cursorY++;
                    }
                    modified = true;
                }
            }
            
            if (ch >= 32 && ch <= 126) { // Printable characters
                if (cursorY >= fileBuffer.size()) {
                    fileBuffer.push_back("");
                }
                fileBuffer[cursorY].insert(cursorX, 1, static_cast<char>(ch));
                cursorX++;
                modified = true;
            }
            
            if (ch == KEY_BACKSPACE || ch == 127) { // 127 is the ASCII DEL code sent by Mac's delete key
                if (cursorY < fileBuffer.size()) {
                    if (cursorX > 0) {
                        fileBuffer[cursorY].erase(cursorX-1, 1);
                        cursorX--;
                        modified = true;
                    } else if (cursorY > 0) {
                        cursorX = fileBuffer[cursorY-1].length();
                        fileBuffer[cursorY-1] += fileBuffer[cursorY];
                        fileBuffer.erase(fileBuffer.begin() + cursorY);
                        cursorY--;
                        modified = true;
                    }
                }
            }
            
            if (ch == KEY_DC) { // Delete
                if (cursorX < fileBuffer[cursorY].length()) {
                    fileBuffer[cursorY].erase(cursorX, 1);
                    modified = true;
                } else if (cursorY < fileBuffer.size() - 1) {
                    fileBuffer[cursorY] += fileBuffer[cursorY + 1];
                    fileBuffer.erase(fileBuffer.begin() + cursorY + 1);
                    modified = true;
                }
            }
            
            if (ch == '\n') {
                std::string remainder = fileBuffer[cursorY].substr(cursorX);
                fileBuffer[cursorY].erase(cursorX);
                fileBuffer.insert(fileBuffer.begin() + cursorY + 1, remainder);
                cursorY++;
                cursorX = 0;
                modified = true;
            }
            
            // Ensure cursor stays within bounds
            if (cursorY >= fileBuffer.size()) {
                cursorY = fileBuffer.size() - 1;
                if (cursorY < 0) cursorY = 0;
            }
            if (cursorY < fileBuffer.size() && cursorX > fileBuffer[cursorY].length()) {
                cursorX = fileBuffer[cursorY].length();
            }
        } else {
            if (ch == 27) break; // ESC to exit
            
            switch(ch) {
                case KEY_UP:
                    if (currentSection == 0 && selectedItem > 0) selectedItem--;
                    else if (currentSection == 1 && subSelectedItem > 0) subSelectedItem--;
                    break;
                case KEY_DOWN:
                    if (currentSection == 0 && selectedItem < dirTree.size() - 1) selectedItem++;
                    else if (currentSection == 1 && subSelectedItem < currentDirContents.size() - 1) subSelectedItem++;
                    break;
                case KEY_RIGHT:
                    if (currentSection < 2) {
                        currentSection++;
                        if (currentSection == 1) {
                            updateSubDir();
                        } else if (currentSection == 2 && subSelectedItem < currentDirContents.size()) {
                            if (!std::filesystem::is_directory(currentDirContents[subSelectedItem])) {
                                fileBuffer.clear();
                                std::ifstream file(currentDirContents[subSelectedItem]);
                                std::string line;
                                while (std::getline(file, line)) {
                                    fileBuffer.push_back(line);
                                }
                                cursorX = 0;
                                cursorY = 0;
                                modified = false;
                            } else {
                                currentSection = 1;
                            }
                        }
                    }
                    break;
                case KEY_LEFT:
                    if (currentSection > 0) currentSection--;
                    break;
                case '\n':
                    if (currentSection == 0) {
                        currentSection = 1;
                        updateSubDir();
                    } else if (currentSection == 1 && subSelectedItem < currentDirContents.size()) {
                        if (!std::filesystem::is_directory(currentDirContents[subSelectedItem])) {
                            currentSection = 2;
                            fileBuffer.clear();
                            std::ifstream file(currentDirContents[subSelectedItem]);
                            std::string line;
                            while (std::getline(file, line)) {
                                fileBuffer.push_back(line);
                            }
                            cursorX = 0;
                            cursorY = 0;
                            modified = false;
                        }
                    }
                    break;
            }
        }
    }
    
    closeEditor();
}
void createNewProject(const std::string& templateName) {
    std::filesystem::path currentPath = std::filesystem::current_path();
    
    if (!std::filesystem::is_empty(currentPath)) {
        std::cout << "Current directory is not empty. Overwrite? (y/n): ";
        char response;
        std::cin >> response;
        
        if (response != 'y' && response != 'Y') {
            std::filesystem::path projectPath = currentPath / templateName;
            std::filesystem::create_directory(projectPath);
            std::cout << "Created new project directory: " << templateName << std::endl;
            return;
        }
        std::filesystem::remove_all(currentPath);
    }
    
    std::cout << "Created new project from template: " << templateName << std::endl;
}

void saveTemplate(const std::string& templateName) {
    std::filesystem::path templateDir = std::filesystem::path(".../templates") / templateName;
    
    if (std::filesystem::exists(templateDir)) {
        std::filesystem::remove_all(templateDir);
    }
    
    std::filesystem::create_directories(templateDir);
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(".")) {
        std::filesystem::path targetPath = templateDir / entry.path().relative_path();
        if (entry.is_directory()) {
            std::filesystem::create_directory(targetPath);
        } else {
            std::filesystem::copy_file(entry.path(), targetPath, std::filesystem::copy_options::overwrite_existing);
        }
    }
    
    std::cout << "Saved current directory as template: " << templateName << std::endl;
}

void generateTemplate(const std::string& projectDescription) {
    std::cout << "Generating template based on description: " << projectDescription << std::endl;
    std::cout << "This feature is current disabled." << std::endl;
}

void openTemplate(const std::string& templateName) {
    std::filesystem::path templateDir = std::filesystem::path("../templates") / templateName;
    
    if (!std::filesystem::exists(templateDir)) {
        std::cout << "Template does not exist: " << templateName << std::endl;
        return;
    }
    
    #ifdef _WIN32
        system(("explorer " + templateDir.string()).c_str());
    #elif __APPLE__
        system(("open " + templateDir.string()).c_str());
    #else
        system(("xdg-open " + templateDir.string()).c_str());
    #endif
    
    std::cout << "Opening template: " << templateName << std::endl;
}

void listTemplates()
{
    std::filesystem::path templateDir = "../templates";

    if (!std::filesystem::exists(templateDir))
    {
        std::cout << "No templates found." << std::endl;
        return;
    }

    std::cout << "Available templates:" << std::endl;
    for (const auto &entry : std::filesystem::directory_iterator(templateDir))
    {
        if (entry.is_directory())
        {
            std::cout << entry.path().filename().string() << std::endl;
        }
    }
}

void deleteTemplate(const std::string& templateName) {
    std::filesystem::path templateDir = std::filesystem::path("../templates") / templateName;
    
    if (!std::filesystem::exists(templateDir)) {
        std::cout << "Template does not exist: " << templateName << std::endl;
        return;
    }
    
    std::filesystem::remove_all(templateDir);
    std::cout << "Deleted template: " << templateName << std::endl;
}

void showUsage() {
    std::cout << "Usage:" << std::endl;
    std::cout << "  templ new <template-name>        Create a new project from a template" << std::endl;
    std::cout << "  templ save <template-name>       Save current directory as a template" << std::endl;
    std::cout << "  templ list                       List available templates"             << std::endl;
    std::cout << "  templ generate <description>     Generate a template using AI"         << std::endl;
    std::cout << "  templ open <template-name>       Open specific template"               << std::endl;
    std::cout << "  templ edit <template-name>       Edit template with built-in editor"   << std::endl;
    std::cout << "  templ delete <template-name>     Delete a template"                    << std::endl;
    std::cout << "  templ help                       Show this help message"               << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        showUsage();
        return 1;
    }

    std::string command = argv[1];
    std::string argument = argc > 2 ? argv[2] : "";

    if (command == "new" && !argument.empty()) {
        createNewProject(argument);
    }
    else if (command == "save" && !argument.empty()) {
        saveTemplate(argument);
    }
    else if (command == "generate" && !argument.empty()) {
        generateTemplate(argument);
    }
    else if (command == "open" && !argument.empty()) {
        openTemplate(argument);
    }
    else if (command == "edit" && !argument.empty()) {
        editTemplate(argument);
    }
    else if (command == "list")
    {
        listTemplates();
    }
    else if (command == "delete")
    {
        deleteTemplate(argument);
    }
    else if (command == "help")
    {
        showUsage();
    }
    else {
        showUsage();
        return 1;
    }

    #ifdef _WIN32
        system("cls");
    #else
        system("reset");
    #endif

    return 0;
}