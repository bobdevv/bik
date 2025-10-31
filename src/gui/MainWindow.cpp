#include "gui/MainWindow.h"
#include "core/BackupManager.h"
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace bik {

MainWindow::MainWindow(int w, int h, const char* title) 
    : Fl_Window(w, h, title), m_manager(new BackupManager()) {
    
    // Set retro color scheme (green on black terminal style)
    color(FL_BLACK);
    
    // Project info at top
    m_projectInfoBox = new Fl_Box(10, 10, w - 20, 60);
    m_projectInfoBox->box(FL_BORDER_BOX);
    m_projectInfoBox->color(fl_rgb_color(20, 20, 20));
    m_projectInfoBox->labelcolor(fl_rgb_color(0, 255, 0));
    m_projectInfoBox->labelfont(FL_COURIER);
    m_projectInfoBox->labelsize(12);
    m_projectInfoBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    if (m_manager->isInitialized()) {
        std::string info = "Project: " + m_manager->getProjectDir() + "\n" +
                          "Backups: " + m_manager->getBackupDir();
        m_projectInfoBox->copy_label(info.c_str());
    } else {
        m_projectInfoBox->label("No project initialized. Use CLI: bik project -b <backup_dir>");
    }
    
    // Backup list browser
    m_backupBrowser = new Fl_Browser(10, 80, w - 20, h - 240);
    m_backupBrowser->type(FL_HOLD_BROWSER);
    m_backupBrowser->color(fl_rgb_color(20, 20, 20));
    m_backupBrowser->textcolor(fl_rgb_color(0, 255, 0));
    m_backupBrowser->textfont(FL_COURIER);
    m_backupBrowser->textsize(12);
    m_backupBrowser->callback(onBrowserSelect, this);
    
    // Details box
    m_detailsBox = new Fl_Box(10, h - 150, w - 20, 80);
    m_detailsBox->box(FL_BORDER_BOX);
    m_detailsBox->color(fl_rgb_color(20, 20, 20));
    m_detailsBox->labelcolor(fl_rgb_color(0, 255, 0));
    m_detailsBox->labelfont(FL_COURIER);
    m_detailsBox->labelsize(11);
    m_detailsBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_TOP);
    m_detailsBox->label("Select a backup to view details");
    
    // Buttons
    int buttonY = h - 60;
    int buttonW = 120;
    int buttonH = 30;
    int spacing = 10;
    
    m_loadButton = new Fl_Button(10, buttonY, buttonW, buttonH, "Load");
    m_loadButton->color(fl_rgb_color(0, 100, 0));
    m_loadButton->labelcolor(fl_rgb_color(0, 255, 0));
    m_loadButton->callback(onLoadBackup, this);
    
    m_createButton = new Fl_Button(10 + buttonW + spacing, buttonY, buttonW, buttonH, "Create");
    m_createButton->color(fl_rgb_color(0, 100, 0));
    m_createButton->labelcolor(fl_rgb_color(0, 255, 0));
    m_createButton->callback(onCreateBackup, this);
    
    m_deleteButton = new Fl_Button(10 + 2 * (buttonW + spacing), buttonY, buttonW, buttonH, "Delete");
    m_deleteButton->color(fl_rgb_color(100, 0, 0));
    m_deleteButton->labelcolor(fl_rgb_color(255, 0, 0));
    m_deleteButton->callback(onDeleteBackup, this);
    
    m_refreshButton = new Fl_Button(10 + 3 * (buttonW + spacing), buttonY, buttonW, buttonH, "Refresh");
    m_refreshButton->color(fl_rgb_color(0, 100, 0));
    m_refreshButton->labelcolor(fl_rgb_color(0, 255, 0));
    m_refreshButton->callback(onRefresh, this);
    
    end();
    resizable(this);
    
    // Load initial data
    refreshBackupList();
}

MainWindow::~MainWindow() {
    delete m_manager;
}

void MainWindow::refreshBackupList() {
    m_backupBrowser->clear();
    
    if (!m_manager->isInitialized()) {
        m_backupBrowser->add("@C1@.No project initialized");
        return;
    }
    
    m_backups = m_manager->listBackups();
    
    if (m_backups.empty()) {
        m_backupBrowser->add("@C1@.No backups found");
        return;
    }
    
    // Add header
    m_backupBrowser->add("@B@.NAME                          | DATE & TIME          | SIZE");
    m_backupBrowser->add("@.─────────────────────────────────────────────────────────────────────────");
    
    for (const auto& backup : m_backups) {
        std::time_t t = backup.timestamp;
        std::tm* tm = std::localtime(&t);
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);
        
        double sizeMB = backup.size / (1024.0 * 1024.0);
        
        std::ostringstream oss;
        oss << std::left << std::setw(30) << backup.name.substr(0, 30)
            << " | " << timeStr
            << " | " << std::fixed << std::setprecision(2) << sizeMB << " MB";
        
        m_backupBrowser->add(oss.str().c_str());
    }
}

void MainWindow::onLoadBackup(Fl_Widget* w, void* data) {
    MainWindow* win = static_cast<MainWindow*>(data);
    win->loadSelectedBackup();
}

void MainWindow::onCreateBackup(Fl_Widget* w, void* data) {
    MainWindow* win = static_cast<MainWindow*>(data);
    win->createNewBackup();
}

void MainWindow::onDeleteBackup(Fl_Widget* w, void* data) {
    MainWindow* win = static_cast<MainWindow*>(data);
    win->deleteSelectedBackup();
}

void MainWindow::onRefresh(Fl_Widget* w, void* data) {
    MainWindow* win = static_cast<MainWindow*>(data);
    win->refreshBackupList();
}

void MainWindow::onBrowserSelect(Fl_Widget* w, void* data) {
    MainWindow* win = static_cast<MainWindow*>(data);
    win->updateDetails();
}

void MainWindow::loadSelectedBackup() {
    int selected = m_backupBrowser->value();
    if (selected <= 2 || selected > static_cast<int>(m_backups.size()) + 2) {
        fl_alert("Please select a backup to load");
        return;
    }
    
    int backupIndex = selected - 3;
    const BackupInfo& backup = m_backups[backupIndex];
    
    int choice = fl_choice("Load backup '%s'?\nThis will replace current directory contents!",
                          "Cancel", "Load", nullptr, backup.name.c_str());
    
    if (choice == 1) {
        // User confirmed
        if (m_manager->loadBackup(backup.name)) {
            fl_message("Backup loaded successfully!");
            refreshBackupList();
        } else {
            fl_alert("Failed to load backup");
        }
    }
}

void MainWindow::createNewBackup() {
    if (!m_manager->isInitialized()) {
        fl_alert("Project not initialized");
        return;
    }
    
    const char* name = fl_input("Enter backup name (leave empty for auto-generated):", "");
    
    if (name == nullptr) {
        return; // Cancelled
    }
    
    std::string backupName = name;
    if (m_manager->createBackup(backupName)) {
        fl_message("Backup created successfully!");
        refreshBackupList();
    } else {
        fl_alert("Failed to create backup");
    }
}

void MainWindow::deleteSelectedBackup() {
    int selected = m_backupBrowser->value();
    if (selected <= 2 || selected > static_cast<int>(m_backups.size()) + 2) {
        fl_alert("Please select a backup to delete");
        return;
    }
    
    int backupIndex = selected - 3;
    const BackupInfo& backup = m_backups[backupIndex];
    
    int choice = fl_choice("Delete backup '%s'?\nThis cannot be undone!",
                          "Cancel", "Delete", nullptr, backup.name.c_str());
    
    if (choice == 1) {
        try {
            std::filesystem::remove(backup.path);
            fl_message("Backup deleted successfully!");
            refreshBackupList();
        } catch (const std::exception& e) {
            fl_alert("Failed to delete backup: %s", e.what());
        }
    }
}

void MainWindow::updateDetails() {
    int selected = m_backupBrowser->value();
    if (selected <= 2 || selected > static_cast<int>(m_backups.size()) + 2) {
        m_detailsBox->label("Select a backup to view details");
        return;
    }
    
    int backupIndex = selected - 3;
    const BackupInfo& backup = m_backups[backupIndex];
    
    std::time_t t = backup.timestamp;
    std::tm* tm = std::localtime(&t);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);
    
    double sizeMB = backup.size / (1024.0 * 1024.0);
    
    std::ostringstream oss;
    oss << "Name: " << backup.name << "\n"
        << "Date: " << timeStr << "\n"
        << "Size: " << std::fixed << std::setprecision(2) << sizeMB << " MB\n"
        << "Path: " << backup.path;
    
    m_detailsBox->copy_label(oss.str().c_str());
}

} // namespace bik
