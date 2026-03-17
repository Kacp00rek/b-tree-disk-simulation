# B-Tree Implementation with Real-Time Visualization

A C++ implementation of a B-Tree data structure designed for disk-based storage simulation. This project includes performance tracking (Disk I/O), automated testing, and a web-based visualization.

**Read the detailed project analysis [here](./docs/report.pdf)**
<p align="center">
  <img src="https://github.com/user-attachments/assets/62c6eaba-0a41-4afc-a14c-d838f460d98a" />
</p>

## Project Structure

- **`src/`**: C++ source files
- **`docs/`**: Project documentation and detailed performance reports
- **`data/`**: Storage for B-Tree data files and generated test scripts
- **`graphs/`**: Graphviz `.dot` files representing the structures for visualizations
- **`images/`**: Rendered PNG from `.dot` files
- **`viewer.html`**: Web tool to view the B-Tree

## Features

- **Disk Simulation**: Monitors **Reads** and **Writes** using a `DiskManager` to simulate physical storage behavior.
- **Cache Simulation**: Optimizes performence by keeping frequently accessed pages in RAM using LRU strategy.
- **Interactive Mode**: Add, remove, or modify records manually with live visualization.
- **Automated Testing**: Generate complex test scenarios with custom operation probabilities and verify output with expected results.
- **Live Visualization**: Generates Graphviz-compatible DOT files after every operation.

## Visualization

- Open `viewer.html` in your browser
- After each operation in the console, refresh the page to see the updated tree
- Alternatively, use the VS Code **Live Server** extension to have the page refresh automatically whenever the `.dot` file changes.
- Requirements: Graphviz

## How to run
- Compile `main.cpp` by using any C++ compiler (supporting C++17 or higher)
- Run the compiled executable


**Autor:** Kacper Grzelakowski  
**GitHub:** [github.com/Kacp00rek](https://github.com/Kacp00rek)
