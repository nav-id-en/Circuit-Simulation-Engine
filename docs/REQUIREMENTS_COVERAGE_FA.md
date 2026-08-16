# تطبیق با صورت پروژه

| الزام | محل پیاده‌سازی |
|---|---|
| زبان C++ | تمام Sourceها با C++20 |
| گرافیک SDL | `src/ui` و هدف `SDL2::SDL2` در CMake |
| جداسازی Frontend/Backend | `ui` مستقل از `core`, `components`, `simulation` |
| صفحه آغازین | `StudioApp::renderWelcome` |
| بوم اصلی | `StudioApp::renderCanvas` |
| Grid و Snap | `renderGrid` و `snap` |
| Zoom و Pan | `handleMouseWheel` و Camera |
| مختصات لحظه‌ای | `renderStatusBar` |
| دسته‌بندی قطعات | Catalog و `categoryFilter_` |
| جست‌وجو | `filteredCatalog` |
| Preview | `catalogIcon` و پنل Preview |
| قرار دادن با Drag & Drop | `libraryDragging_` و `addComponent` |
| انتخاب تکی/چندگانه/ناحیه‌ای | Selection Set و Selection Box |
| حرکت قطعه | `movingComponents_` |
| Rotate و Mirror | `rotateSelection`, `mirrorSelection` |
| Properties | پنل Properties و Modal Editor |
| تشخیص Pin | `pinAt` |
| Wire 90 درجه | `finishWire` و `wirePath` |
| Junction Dot | `createJunction` و `Circuit::addJunction` |
| Wire Dragging | محاسبه مجدد Endpoint در `wirePath` |
| حذف Wire | Hit Test سیم و `deleteSelection` |
| GND، DC، Battery، Clock | `Components.cpp` |
| R، C، L | Stampهای آنالوگ در `Components.cpp` |
| Switch، Push Button، LED، 7-Segment | مدل و تعامل Runtime |
| AND، OR، NOT، XOR، NAND | `LogicGateComponent` |
| D Flip-Flop | `DFlipFlopComponent` |
| Undefined و Warning | `Signal`, Gate Evaluate و Log |
| Propagation Delay | Event Queue در `SimulationEngine` |
| ADC و DAC | `AdcComponent`, `DacComponent` |
| Intel HEX | `FirmwareLoader` |
| MCU و دستورها | `MicrocontrollerComponent` |
| External Memory | `ExternalMemoryComponent` |
| LCD 16x2 | `LcdComponent` |
| Keypad 4x4 | `KeypadComponent` |
| Run/Pause/Stop | `SimulationEngine` و Toolbar |
| Live Wire Animation | `wireColor` |
| تعامل حین Run | `handleRuntimePress` |
| Step | `SimulationEngine::step` |
| Voltage Probe | `addProbe` |
| Voltmeter/Ammeter | Componentهای ابزار |
| Oscilloscope چندکاناله | `renderScope` |
| Time/Div و Volts/Div | کنترل‌های Scope |
| Save/Open/Recent | `CircuitSerializer` و Recent list |
| Save As | Modal مسیر و بررسی Overwrite |
| ذخیره وضعیت اجرا | Runtime toggle و schemaVersion 3 |
| Undo/Redo | History Snapshot |
| Export PNG | PNG encoder و `exportImage` |
| DRC اتصال کوتاه | `OUTPUT_SHORT` |
| DRC Floating | `FLOATING_INPUT` |
| Simulation Log | پنل پایین و `LogEntry` |

