# NS-3 5GLENA V2X/V2I/MEC Simulation Script (`cttc-nr-v2x-mec`)

This project provides an advanced NS-3 simulation script for evaluating 5G New Radio V2X (Vehicle-to-Everything) and V2I (Vehicle-to-Infrastructure) communication scenarios with mobility and MEC (Multi-access Edge Computing) features. The script leverages the [5GLENA](https://5g-lena.cttc.es/) module for 5G NR, enabling detailed configuration of sidelink (SL) features, mobility models, and application layer traffic.

---

## Key Features

- **5G NR V2X and V2I Simulation:** Uses NS-3 and 5GLENA for realistic vehicular communication scenarios.
- **Mobility Integration:** Reads NS2 mobility traces for vehicles and GNb positions, enabling urban movement patterns.
- **Sidelink (SL) Configuration:** Customizable SL resource pools, bearers, and traffic flow templates for groupcast communication.
- **Application Layer Traffic:** UDP multicast communication with configurable data rate and packet size.
- **Performance Metrics:** Automatic calculation of throughput, transmitted/received bits and packets.
- **Flexible Configuration:** Simulation parameters (mobility files, GNb positions, output directory, seed, etc.) are set via command-line arguments.
- **Modular Design:** Functions are provided for band configuration, EPC/NR setup, SL pool creation, and more.

---

## Requirements

- **NS-3** (latest release, built with CMake)
- **5GLENA module** installed and configured
- C++17 compatible compiler (e.g., g++)
- NS-3 development dependencies
- Python (optional, for post-processing statistics)
- Mobility trace files in NS2 format for vehicles and GNb positions

---

## Installation & Setup

1. **Install NS-3 and 5GLENA:**
   - Follow the [official 5GLENA installation instructions](https://5g-lena.cttc.es/).
2. **Add the script to NS-3:**
   - Place `cttc-nr-v2x-mec.cc` inside the `scratch` directory of your NS-3 source tree:
     ```bash
     mv cttc-nr-v2x-mec.cc /path/to/ns-3/scratch/
     ```
   - Make sure any custom headers (e.g., `mob-utils.h`) are also available in your NS-3 workspace.
3. **Build NS-3:**
   ```bash
   cd /path/to/ns-3/
   ./ns3 configure
   ./ns3 build
   ```

---

## Usage

To run the simulation, use the following command from the NS-3 source directory:

```bash
./ns3 run scratch/cttc-nr-v2x-mec \
  --logging=true \
  --mobilityFile=urban-low.tcl \
  --GNbPositions=001-gnb.tcl \
  --outputDir=./results \
  --seed=1
```

### Command-Line Parameters

- `logging` (bool): Enable logging output.
- `mobilityFile` (string): NS2 trace file for vehicle mobility.
- `GNbPositions` (string): NS2 trace file for GNb positions.
- `outputDir` (string): Directory for simulation results.
- `seed` (uint32): RNG seed for reproducible runs.

---

## Input Files

- Place mobility and GNb position trace files (in NS2 `.tcl` format) under `scratch/mob/`.
  - Example: `scratch/mob/urban-low.tcl`, `scratch/mob/001-gnb.tcl`

---

## Output & Results

- Simulation results (throughput, packets, bits, etc.) are printed to the console and can be redirected to files.
- Application-layer statistics (Rx/Tx packets/bits, average throughput) are shown at the end of the run.
- Further analysis can be performed using NS-3's FlowMonitor or custom Python scripts.

---

## References

- [NS-3 Official Site](https://www.nsnam.org/)
- [5GLENA Module](https://5g-lena.cttc.es/)
- [NS2 Mobility Format](https://www.nsnam.org/docs/models/html/mobility.html#ns2-mobility-helper)

---

## License

This project is licensed under the GNU GPL v2.0. See the header in `cttc-nr-v2x-mec.cc` for details.

---

## Author

Sérgio Vieira  
Federal Institute of Education, Science and Technology of Ceará  
Contact: sergio.vieira@ifce.edu.br
