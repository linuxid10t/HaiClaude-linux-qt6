/*  LauncherWindow.cpp – HaiClaude (Linux/Qt6) */

#include "LauncherWindow.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>

#include <unistd.h>

// ---------------------------------------------------------------------------
// Helper: Shell Escape
// ---------------------------------------------------------------------------

static QString shellEscape(const QString& s)
{
    return "'" + QString(s).replace("'", "'\\''") + "'";
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

LauncherWindow::LauncherWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("Claude Code Launcher");

    // --- Mode radios (exclusive group: Cloud vs API) ---
    fModeGroup  = new QButtonGroup(this);
    fCloudRadio = new QRadioButton("Cloud");
    fApiRadio   = new QRadioButton("API  (API key)");
    fModeGroup->addButton(fCloudRadio, 0);
    fModeGroup->addButton(fApiRadio,   1);
    fCloudRadio->setChecked(true);

    // --- Cloud model radios (separate exclusive group: Opus/Sonnet/Haiku) ---
    fModelGroup      = new QButtonGroup(this);
    fCloudOpusRadio   = new QRadioButton("Opus");
    fCloudSonnetRadio = new QRadioButton("Sonnet");
    fCloudHaikuRadio  = new QRadioButton("Haiku");
    fModelGroup->addButton(fCloudOpusRadio,   0);
    fModelGroup->addButton(fCloudSonnetRadio, 1);
    fModelGroup->addButton(fCloudHaikuRadio,  2);
    fCloudOpusRadio->setChecked(true);

    fModelBox = new QGroupBox("Model");
    QHBoxLayout* modelLayout = new QHBoxLayout(fModelBox);
    modelLayout->addStretch();
    modelLayout->addWidget(fCloudOpusRadio);
    modelLayout->addWidget(fCloudSonnetRadio);
    modelLayout->addWidget(fCloudHaikuRadio);
    modelLayout->addStretch();

    // --- Working directory row ---
    fWorkDirField = new QLineEdit(QDir::homePath());
    fBrowseBtn    = new QPushButton("Browse\xe2\x80\xa6");
    fBrowseBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // --- API settings box ---
    fApiBox = new QGroupBox("API Settings");
    QFormLayout* apiLayout = new QFormLayout(fApiBox);
    apiLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    fApiUrlField = new QLineEdit("https://api.anthropic.com/v1");
    apiLayout->addRow("API URL:", fApiUrlField);

    fApiKeyField = new QLineEdit;
    fApiKeyField->setEchoMode(QLineEdit::Password);
    apiLayout->addRow("API Key:", fApiKeyField);

    fSaveApiKeyCheck = new QCheckBox("Remember key");
    apiLayout->addRow("", fSaveApiKeyCheck);

    // Helper to add a checkbox + hidden field row
    auto addOverrideRow = [&](const QString& label, const QString& defaultVal,
                               QCheckBox*& check, QLineEdit*& field) {
        check = new QCheckBox(label);
        field = new QLineEdit(defaultVal);
        field->setVisible(false);
        apiLayout->addRow(check);
        apiLayout->addRow(field);
    };

    addOverrideRow("Override current model",                  "claude-sonnet-4-6",
                   fApiCurrentModelCheck, fApiCurrentModelField);
    addOverrideRow("Override ANTHROPIC_DEFAULT_OPUS_MODEL",   "claude-opus-4-20250514",
                   fApiOpusModelCheck,    fApiOpusModelField);
    addOverrideRow("Override ANTHROPIC_DEFAULT_SONNET_MODEL", "claude-sonnet-4-6",
                   fApiSonnetModelCheck,  fApiSonnetModelField);
    addOverrideRow("Override ANTHROPIC_DEFAULT_HAIKU_MODEL",  "claude-haiku-4-20250514",
                   fApiHaikuModelCheck,   fApiHaikuModelField);

    // --- Launch button ---
    fLaunchBtn = new QPushButton("Launch Claude Code");
    fLaunchBtn->setDefault(true);

    // --- Root layout ---
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setSpacing(6);
    root->setContentsMargins(12, 12, 12, 12);

    root->addWidget(new QLabel("Launch mode:"));

    // Cloud radio (indented)
    QHBoxLayout* cloudRow = new QHBoxLayout;
    cloudRow->setContentsMargins(16, 0, 0, 0);
    cloudRow->addWidget(fCloudRadio);
    cloudRow->addStretch();
    root->addLayout(cloudRow);

    root->addWidget(fModelBox);

    // API radio (indented)
    QHBoxLayout* apiRow = new QHBoxLayout;
    apiRow->setContentsMargins(16, 0, 0, 0);
    apiRow->addWidget(fApiRadio);
    apiRow->addStretch();
    root->addLayout(apiRow);

    // Directory row
    QHBoxLayout* dirRow = new QHBoxLayout;
    dirRow->addWidget(new QLabel("Directory:"));
    dirRow->addWidget(fWorkDirField);
    dirRow->addWidget(fBrowseBtn);
    root->addLayout(dirRow);

    root->addWidget(fApiBox);
    root->addStretch();
    root->addWidget(fLaunchBtn);

    // --- Connections ---
    connect(fModeGroup, &QButtonGroup::buttonToggled,
            this, [this](QAbstractButton*, bool) { updateModeVisibility(); });

    connect(fBrowseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(
            this, "Select Working Directory",
            fWorkDirField->text(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty())
            fWorkDirField->setText(dir);
    });

    connect(fLaunchBtn, &QPushButton::clicked, this, [this]() {
        // Validate working directory
        QString workDir = fWorkDirField->text().trimmed();
        if (!workDir.isEmpty() && !QDir(workDir).exists()) {
            QMessageBox::warning(this, "Invalid Directory",
                "The specified working directory does not exist:\n" + workDir);
            return;
        }

        // Validate API fields if in API mode
        if (fApiRadio->isChecked()) {
            if (fApiUrlField->text().trimmed().isEmpty()) {
                QMessageBox::warning(this, "Missing API URL",
                    "API URL cannot be empty.");
                return;
            }
            if (fApiKeyField->text().isEmpty()) {
                QMessageBox::warning(this, "Missing API Key",
                    "API Key cannot be empty.");
                return;
            }
        }

        saveSettings();
        QString cmd = buildCommand();
        if (isatty(STDIN_FILENO)) {
            gPendingExec = cmd.toStdString();
        } else {
            launchInTerminal(cmd);
        }
        close();
    });

    auto connectCheck = [this](QCheckBox* cb, QLineEdit* field) {
        connect(cb, &QCheckBox::toggled, this, [this, field](bool checked) {
            field->setVisible(checked);
            resizeToFit();
        });
    };
    connectCheck(fApiCurrentModelCheck, fApiCurrentModelField);
    connectCheck(fApiOpusModelCheck,    fApiOpusModelField);
    connectCheck(fApiSonnetModelCheck,  fApiSonnetModelField);
    connectCheck(fApiHaikuModelCheck,   fApiHaikuModelField);

    // --- Initial state ---
    loadSettings();
    updateModeVisibility(); // calls resizeToFit()

    // Center on screen
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen)
        move(screen->geometry().center() - rect().center());
}

// ---------------------------------------------------------------------------
// resizeToFit
// ---------------------------------------------------------------------------

void LauncherWindow::resizeToFit()
{
    // Unlock size limits so adjustSize() can change both dimensions
    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    layout()->activate();
    adjustSize();
    setFixedSize(size());
}

// ---------------------------------------------------------------------------
// updateModeVisibility
// ---------------------------------------------------------------------------

void LauncherWindow::updateModeVisibility()
{
    bool isApi = fApiRadio->isChecked();
    fModelBox->setVisible(!isApi);
    fApiBox->setVisible(isApi);
    resizeToFit();
}

// ---------------------------------------------------------------------------
// buildCommand
// ---------------------------------------------------------------------------

QString LauncherWindow::buildCommand() const
{
    QString claudeBin = findClaude();
    QString workDir   = fWorkDirField->text().trimmed();
    QString cmd;

    if (!workDir.isEmpty())
        cmd += "cd " + shellEscape(workDir) + " && ";

    if (fCloudRadio->isChecked()) {
        cmd += "unset ANTHROPIC_API_KEY && " + claudeBin;

        int modelId = fModelGroup->checkedId();
        if      (modelId == 1) cmd += " --model sonnet";
        else if (modelId == 2) cmd += " --model haiku";
        else                   cmd += " --model opus";

    } else {
        QString apiUrl = fApiUrlField->text();
        QString apiKey = fApiKeyField->text();

        // Use trap to guarantee credentials are restored on exit
        cmd += "trap 'mv \"$HOME/.claude/.credentials.json.bak\" "
               "\"$HOME/.claude/.credentials.json\" 2>/dev/null' EXIT; ";
        cmd += "mv \"$HOME/.claude/.credentials.json\" "
               "\"$HOME/.claude/.credentials.json.bak\" 2>/dev/null; ";
        cmd += "ANTHROPIC_BASE_URL=" + shellEscape(apiUrl) +
               " ANTHROPIC_API_KEY=" + shellEscape(apiKey);

        auto addEnv = [&](QCheckBox* cb, QLineEdit* field, const QString& var) {
            if (cb->isChecked() && !field->text().isEmpty())
                cmd += " " + var + "=" + shellEscape(field->text());
        };
        addEnv(fApiOpusModelCheck,   fApiOpusModelField,   "ANTHROPIC_DEFAULT_OPUS_MODEL");
        addEnv(fApiSonnetModelCheck, fApiSonnetModelField, "ANTHROPIC_DEFAULT_SONNET_MODEL");
        addEnv(fApiHaikuModelCheck,  fApiHaikuModelField,  "ANTHROPIC_DEFAULT_HAIKU_MODEL");

        cmd += " " + claudeBin;

        if (fApiCurrentModelCheck->isChecked() && !fApiCurrentModelField->text().isEmpty())
            cmd += " --model " + shellEscape(fApiCurrentModelField->text());
    }

    return cmd;
}

// ---------------------------------------------------------------------------
// findClaude
// ---------------------------------------------------------------------------

QString LauncherWindow::findClaude() const
{
    const QStringList candidates = {
        QDir::homePath() + "/.npm-global/bin/claude",
        QDir::homePath() + "/.local/bin/claude",
        "/usr/local/bin/claude",
    };
    for (const QString& path : candidates) {
        if (QFile::exists(path))
            return path;
    }
    // Fall back to PATH lookup; the login shell will resolve it
    const QString found = QStandardPaths::findExecutable("claude");
    return found.isEmpty() ? QStringLiteral("claude") : found;
}

// ---------------------------------------------------------------------------
// findTerminal
// ---------------------------------------------------------------------------

QString LauncherWindow::findTerminal() const
{
    // 1. Honour $TERMINAL if set
    const QString envTerm = qEnvironmentVariable("TERMINAL");
    if (!envTerm.isEmpty()) {
        const QString path = QStandardPaths::findExecutable(envTerm);
        if (!path.isEmpty())
            return path;
    }

    // 2. Common terminal emulators in order of preference
    const QStringList candidates = {
        "xterm", "alacritty", "kitty",
        "gnome-terminal", "konsole",
        "xfce4-terminal", "lxterminal", "x-terminal-emulator",
    };
    for (const QString& name : candidates) {
        const QString path = QStandardPaths::findExecutable(name);
        if (!path.isEmpty())
            return path;
    }
    return QString();
}

// ---------------------------------------------------------------------------
// launchInTerminal
// ---------------------------------------------------------------------------

void LauncherWindow::launchInTerminal(const QString& cmd)
{
    const QString termPath = findTerminal();
    if (termPath.isEmpty()) {
        QMessageBox::critical(this, "No Terminal Found",
            "Could not find a terminal emulator.\n"
            "Please install xterm, alacritty, konsole, or gnome-terminal,\n"
            "or set the $TERMINAL environment variable.");
        return;
    }

    const QString termName = QFileInfo(termPath).fileName();

    // Use a login shell so the user's PATH and env are fully initialised
    QStringList args;
    if (termName == "gnome-terminal")
        args = {"--", "bash", "-l", "-c", cmd};
    else if (termName == "kitty")
        args = {"bash", "-l", "-c", cmd};
    else
        args = {"-e", "bash", "-l", "-c", cmd};

    QProcess::startDetached(termPath, args);
}

// ---------------------------------------------------------------------------
// loadSettings / saveSettings
// ---------------------------------------------------------------------------

void LauncherWindow::loadSettings()
{
    QSettings s;

    const int mode = s.value("mode", 0).toInt();
    if (mode == 1) fApiRadio->setChecked(true);
    else           fCloudRadio->setChecked(true);

    const int cloudModel = s.value("cloudModel", 0).toInt();
    if      (cloudModel == 1) fCloudSonnetRadio->setChecked(true);
    else if (cloudModel == 2) fCloudHaikuRadio->setChecked(true);
    else                      fCloudOpusRadio->setChecked(true);

    fWorkDirField->setText(s.value("workDir", QDir::homePath()).toString());
    fApiUrlField->setText( s.value("apiUrl",  "https://api.anthropic.com/v1").toString());

    // Only load API key if saveApiKey is true
    const bool saveKey = s.value("saveApiKey", false).toBool();
    fSaveApiKeyCheck->setChecked(saveKey);
    if (saveKey)
        fApiKeyField->setText(s.value("apiKey", "").toString());

    auto loadCheck = [&](const QString& checkKey, QCheckBox* cb,
                         const QString& fieldKey, const QString& defaultVal, QLineEdit* field) {
        const bool checked = s.value(checkKey, false).toBool();
        cb->setChecked(checked);
        field->setVisible(checked);
        field->setText(s.value(fieldKey, defaultVal).toString());
    };

    loadCheck("apiCurrentModelCheck", fApiCurrentModelCheck,
              "apiCurrentModel", "claude-sonnet-4-6",      fApiCurrentModelField);
    loadCheck("apiOpusModelCheck",    fApiOpusModelCheck,
              "apiOpusModel",    "claude-opus-4-20250514",  fApiOpusModelField);
    loadCheck("apiSonnetModelCheck",  fApiSonnetModelCheck,
              "apiSonnetModel",  "claude-sonnet-4-6",      fApiSonnetModelField);
    loadCheck("apiHaikuModelCheck",   fApiHaikuModelCheck,
              "apiHaikuModel",   "claude-haiku-4-20250514", fApiHaikuModelField);
}

void LauncherWindow::saveSettings()
{
    QSettings s;

    s.setValue("mode",      fApiRadio->isChecked() ? 1 : 0);
    const int cloudModel = fModelGroup->checkedId();
    s.setValue("cloudModel", cloudModel >= 0 ? cloudModel : 0);

    s.setValue("workDir",  fWorkDirField->text());
    s.setValue("apiUrl",   fApiUrlField->text());

    // Only save API key if checkbox is checked
    s.setValue("saveApiKey", fSaveApiKeyCheck->isChecked());
    if (fSaveApiKeyCheck->isChecked())
        s.setValue("apiKey", fApiKeyField->text());

    s.setValue("apiCurrentModelCheck", fApiCurrentModelCheck->isChecked());
    s.setValue("apiCurrentModel",      fApiCurrentModelField->text());
    s.setValue("apiOpusModelCheck",    fApiOpusModelCheck->isChecked());
    s.setValue("apiOpusModel",         fApiOpusModelField->text());
    s.setValue("apiSonnetModelCheck",  fApiSonnetModelCheck->isChecked());
    s.setValue("apiSonnetModel",       fApiSonnetModelField->text());
    s.setValue("apiHaikuModelCheck",   fApiHaikuModelCheck->isChecked());
    s.setValue("apiHaikuModel",        fApiHaikuModelField->text());
}
