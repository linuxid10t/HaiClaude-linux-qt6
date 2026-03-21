#pragma once

#include <QWidget>

#include <string>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;

extern std::string gPendingExec;

class LauncherWindow : public QWidget {
    Q_OBJECT

public:
    explicit LauncherWindow(QWidget* parent = nullptr);

private:
    void updateModeVisibility();
    void resizeToFit();
    void loadSettings();
    void saveSettings();
    QString buildCommand() const;
    QString findClaude() const;
    QString findTerminal() const;
    void launchInTerminal(const QString& cmd);

    // Profile management
    void loadProfiles();
    void saveCurrentProfile();
    void deleteProfile();
    void loadProfile(const QString& name);
    void updateProfileComboBox();
    void updateProfileListWidget();

    QButtonGroup*   fModeGroup;
    QRadioButton*   fCloudRadio;
    QRadioButton*   fApiRadio;

    QGroupBox*      fModelBox;
    QButtonGroup*   fModelGroup;
    QRadioButton*   fCloudOpusRadio;
    QRadioButton*   fCloudSonnetRadio;
    QRadioButton*   fCloudHaikuRadio;

    QLineEdit*      fWorkDirField;
    QPushButton*    fBrowseBtn;

    QGroupBox*      fApiBox;
    QGroupBox*      fProfileBox;
    QLineEdit*      fApiUrlField;
    QLineEdit*      fApiKeyField;
    QCheckBox*      fSaveApiKeyCheck;
    QCheckBox*      fApiCurrentModelCheck;
    QLineEdit*      fApiCurrentModelField;
    QCheckBox*      fApiOpusModelCheck;
    QLineEdit*      fApiOpusModelField;
    QCheckBox*      fApiSonnetModelCheck;
    QLineEdit*      fApiSonnetModelField;
    QCheckBox*      fApiHaikuModelCheck;
    QLineEdit*      fApiHaikuModelField;

    // Profile management
    QComboBox*      fProfileComboBox;
    QListWidget*    fProfileListWidget;
    QPushButton*    fSaveProfileButton;
    QPushButton*    fDeleteProfileButton;
    QLineEdit*      fProfileNameEdit;

    QPushButton*    fLaunchBtn;
};
