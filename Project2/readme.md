# CPU-Simulator using Windows Forms

This project provides a Windows Forms application that demonstrates common CPU scheduling algorithms through an interactive graphical interface. Each algorithm prompts for basic input and displays the resulting waiting or turnaround times using message boxes and on-screen tables.

**Fork maintained by Chris Regan** - Original creator: Francis (used with permission)

## Project status

The simulator is functional but still a work in progress. Currently the following scheduling strategies are available:

| Algorithm | Method | Notes |
|-----------|--------|-------|
| First Come First Serve | `Algorithms.RunFirstComeFirstServe` | Processes are executed in order of arrival. |
| Shortest Job First | `Algorithms.RunShortestJobFirst` | Jobs are sorted by burst time before execution. |
| Priority Scheduling | `Algorithms.RunPriorityScheduling` | User supplies a priority value for each job. |
| Round Robin | `Algorithms.RunRoundRobin` | Requires a quantum time parameter. |

Additional algorithms can easily be added by extending `Algorithms.cs`.

## Requirements

- Windows operating system
- .NET 8.0 SDK or newer
- Visual Studio 2022 or VS Code with C# extensions

## How to run

### Using Visual Studio

1. Clone the repository:

   ```bash
   git clone git@github.com:iAmGiG/CS-3502-CPU-Sim-Project-StartingPoint.git
   ```

2. Open `CpuScheduler.sln` in Visual Studio 2022
3. Press F5 to build and run the application

### Using VS Code

1. Clone the repository:

   ```bash
   git clone git@github.com:iAmGiG/CS-3502-CPU-Sim-Project-StartingPoint.git
   ```

2. Install the C# Dev Kit extension in VS Code

3. Open the project folder in VS Code

4. **Option A - Using the Debugger (Recommended):**
   - Press **F5** or go to Run & Debug panel
   - Select ".NET Core Launch (console)" configuration
   - This will build and launch the Windows Forms app with debugging support

5. **Option B - Using Terminal (May have termination issues):**

   ```bash
   dotnet build
   dotnet run --project CpuScheduler/CpuScheduler.csproj
   ```

   **Note:** Windows Forms apps may not terminate cleanly in VS Code's integrated terminal

6. **Option C - Run the Built Executable Directly:**

   ```bash
   dotnet build
   # Then navigate to: CpuScheduler/bin/Debug/net8.0-windows/CpuScheduler.exe
   # Double-click the .exe file or run from command prompt
   ```

### Using .NET CLI

From the project root directory:

```bash
# Build the project
dotnet build

# Run the application
dotnet run --project CpuScheduler/CpuScheduler.csproj
```

## Usage

1. Enter the desired number of processes
2. Choose a scheduling algorithm from the interface
3. The app will prompt for additional values as needed (burst time, priority, quantum time, etc.)
4. View the results in the display table showing waiting times and turnaround times

### License

This project is licensed under the terms of the [MIT license](LICENSE.txt).
