#include <atomic>
#include <chrono>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

// ============================================================================
// PRUNE UCI TRANSPORT DIAGNOSTIC
// ============================================================================
//
// This is intentionally NOT the Prune chess engine.
//
// It is a temporary process/GUI communication probe.  It removes all Prune
// engine initialization, NNUE, SIMD/NEON, search, hash tables, Fathom,
// constructors, and other engine code from the executable while preserving
// the same Android build/workflow entry point (core/engine.cpp).
//
// Purpose:
//   Determine whether CET can install/start/communicate with an executable
//   produced by the Prune Android build environment.
//
// If CET accepts this binary, the Android process/pipe itself is working and
// the failure is inside Prune runtime initialization.  If CET still cannot
// install/start it, investigate the Android binary/launcher/CET boundary.
//
// ============================================================================

namespace {

std::atomic<bool> quit_requested{false};
std::atomic<bool> stop_requested{false};
std::thread search_thread;

void join_search()
{
    if (search_thread.joinable())
        search_thread.join();
}

void print_uci()
{
    std::cout << "id name Prune-CET-Diagnostic" << std::endl;
    std::cout << "id author Prune" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
    std::cout << "uciok" << std::endl;
}

void start_dummy_search()
{
    join_search();

    stop_requested.store(false, std::memory_order_release);

    search_thread = std::thread([] {
        // Do not perform any chess-engine initialization.  This merely keeps
        // the process behavior representative of an engine that can search
        // asynchronously while remaining responsive to "stop".
        while (!stop_requested.load(std::memory_order_acquire) &&
               !quit_requested.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            break;
        }

        if (!quit_requested.load(std::memory_order_acquire)) {
            std::cout << "bestmove e2e4" << std::endl;
        }
    });
}

void handle_command(const std::string& command)
{
    if (command == "uci") {
        print_uci();
        return;
    }

    if (command == "isready") {
        std::cout << "readyok" << std::endl;
        return;
    }

    if (command == "ucinewgame") {
        stop_requested.store(true, std::memory_order_release);
        join_search();
        return;
    }

    if (command == "stop") {
        stop_requested.store(true, std::memory_order_release);
        join_search();
        return;
    }

    if (command == "quit") {
        quit_requested.store(true, std::memory_order_release);
        stop_requested.store(true, std::memory_order_release);
        join_search();
        return;
    }

    if (command.rfind("setoption", 0) == 0) {
        // Accept arbitrary UCI options.  The diagnostic deliberately does
        // nothing with them.
        return;
    }

    if (command.rfind("position", 0) == 0) {
        // Accept arbitrary position commands without parsing or initializing
        // Prune's GameState.
        return;
    }

    if (command.rfind("go", 0) == 0) {
        start_dummy_search();
        return;
    }

    // CET/GUI probing commands that do not require a chess implementation.
    if (command == "ponderhit")
        return;

    // Unknown commands are intentionally ignored, as required by robust UCI
    // front ends.
}

} // namespace

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    // Android GUI engines communicate through pipes.  Make both streams
    // unbuffered so no UCI response can remain stuck in a stdio buffer.
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    quit_requested.store(false, std::memory_order_release);
    stop_requested.store(false, std::memory_order_release);

    std::string line;

    while (!quit_requested.load(std::memory_order_acquire) &&
           std::getline(std::cin, line)) {

        // Normalize CRLF and surrounding whitespace.
        while (!line.empty() &&
               std::isspace(static_cast<unsigned char>(line.back()))) {
            line.pop_back();
        }

        std::size_t first = 0;
        while (first < line.size() &&
               std::isspace(static_cast<unsigned char>(line[first]))) {
            ++first;
        }

        if (first != 0)
            line.erase(0, first);

        if (!line.empty())
            handle_command(line);
    }

    // EOF is treated exactly like quit.  This is important when the GUI closes
    // its write end of the engine pipe.
    quit_requested.store(true, std::memory_order_release);
    stop_requested.store(true, std::memory_order_release);
    join_search();

    return 0;
}
