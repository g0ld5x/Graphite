#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
    #include <conio.h>   // Native Windows console header
    #include <windows.h> // For Sleep
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <termios.h>
#endif

// Cross-platform sleep function helper
void universal_sleep(int milliseconds) {
    #if defined(_WIN32) || defined(_WIN64)
        Sleep(milliseconds);
    #else
        usleep(milliseconds * 1000);
    #endif
}

class CrossPlatformKeyboard {
private:
#if !defined(_WIN32) && !defined(_WIN64)
    int tty_fd = -1;
    struct termios original_settings;
#endif

public:
    CrossPlatformKeyboard() {
        #if !defined(_WIN32) && !defined(_WIN64)
            // Linux/macOS initialization
            tty_fd = open("/dev/tty", O_RDONLY | O_NONBLOCK);
            if (tty_fd >= 0) {
                struct termios raw_settings;
                tcgetattr(tty_fd, &original_settings);
                raw_settings = original_settings;
                raw_settings.c_lflag &= ~(ICANON | ECHO); 
                raw_settings.c_cc[VMIN] = 1;
                raw_settings.c_cc[VTIME] = 0;
                tcsetattr(tty_fd, TCSANOW, &raw_settings);
            }
        #endif
    }

    ~CrossPlatformKeyboard() {
        #if !defined(_WIN32) && !defined(_WIN64)
            // Linux/macOS cleanup
            if (tty_fd >= 0) {
                tcsetattr(tty_fd, TCSANOW, &original_settings);
                close(tty_fd);
            }
        #endif
    }

    // Returns character if pressed, or 0 if no key was pressed
    char get_key() {
        #if defined(_WIN32) || defined(_WIN64)
            // Windows implementation
            if (_kbhit()) {
                return static_cast<char>(_getch());
            }
        #else
            // Linux/macOS implementation
            if (tty_fd >= 0) {
                char ch;
                if (read(tty_fd, &ch, 1) > 0) {
                    return ch;
                }
            }
        #endif
        return 0; // No key pressed
    }
};

int main() {
    std::cout << "Cross-platform raw input active. Press 'q' to quit.\n";
    
    CrossPlatformKeyboard keyboard;

    while (true) {
        char ch = keyboard.get_key();
        
        if (ch != 0) {
            std::cout << "\r\nCaught key: " << ch << std::flush;
            
            if (ch == 'q' || ch == 'Q') {
                break;
            }
        }
        
        universal_sleep(10); // Prevents 100% CPU usage on all platforms
    }

    std::cout << "\nExiting cleanly.\n";
    return 0;
}
