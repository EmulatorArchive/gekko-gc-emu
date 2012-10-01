#include <QThread>
#include <QGLWidget>
#include "platform.h"
#include "video/emuwindow.h"

class GRenderWindow;
class QKeyEvent;

class EmuThread : public QThread
{
    Q_OBJECT

public:
    /**
     * Set image filename
     * 
     * @param filename
     * @warning Only call when not running!
     */
    void SetFilename(const char* filename);

    /**
     * Start emulation (on new thread)
     *
     * @warning Only call when not running!
     */
    void run();

    /**
     * Allow the CPU to process a single instruction (if cpu is not running)
     *
     * @note This function is thread-safe
     */
    void ExecStep() { exec_cpu_step = true; }

    /**
     * Allow the CPU to continue processing instructions without interruption
     *
     * @note This function is thread-safe
     */
    void SetCpuRunning(bool running) { cpu_running = running; }

public slots:
    /**
     * Stop emulation and wait for the thread to finish.
     *
     * @details: This function will wait a second for the thread to finish; if it hasn't finished until then, we'll terminate() it and wait another second, hoping that it will be terminated by then.
     * @note: This function is thread-safe.
     */
    void Stop();

private:
    friend class GRenderWindow;

    EmuThread(GRenderWindow* render_window);

    char filename[MAX_PATH];

    bool exec_cpu_step;
    bool cpu_running;

    GRenderWindow* render_window;

signals:
    /**
     * Emitted when CPU when we've finished processing a single Gekko instruction
     * 
     * @warning This will only be emitted when the CPU is not running (SetCpuRunning(false))
     * @warning When connecting to this signal from other threads, make sure to specify either Qt::QueuedConnection (invoke slot within the destination object's message thread) or even Qt::BlockingQueuedConnection (additionally block source thread until slot returns)
     */
    void CPUStepped();
};

class GRenderWindow : public QWidget, public EmuWindow
{
public:
    GRenderWindow(QWidget* parent = NULL);
    ~GRenderWindow();

    void closeEvent(QCloseEvent*);

    // EmuWindow implementation
    void SwapBuffers();
    void SetTitle(const char* title);
    void MakeCurrent();
    void DoneCurrent();
    void GetWindowSize(int &width, int &height) {}
    void SetConfig(EmuWindow::Config config) {}
    void PollEvents() {}

    void BackupGeometry();
    void RestoreGeometry();
    void restoreGeometry(const QByteArray& geometry); // overridden
    QByteArray saveGeometry(); // overridden

    EmuThread& GetEmuThread();

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

private:
    QGLWidget* child;

    EmuThread emu_thread;

    QByteArray geometry;
};
