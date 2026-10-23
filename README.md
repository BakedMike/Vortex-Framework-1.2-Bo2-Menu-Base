# Vortex Framework 1.2 Base

A recreation of the PS3 "Vortex Framework 1.2" mod menu for the **Xbox 360**, built for **Call of Duty: Black Ops II Multiplayer (TU18)**.

<img width="800" height="450" alt="d5b958d4-91ae-4db7-9204-218a754c3b22" src="https://github.com/user-attachments/assets/4ec43eae-24b1-4057-af7b-21fdbc5cbde2" />

## How to open the menu

Hold **LB** and press **D-Pad Down**. Use the same combination to close it again.

While the menu is open the game ignores your buttons and triggers. You can still walk and look around with the sticks.

| Button | Action |
|---|---|
| D-Pad Up / Down | Move the selection |
| D-Pad Left / Right | Change dropdown and slider values |
| A | Select / toggle / open a dropdown or submenu |
| B | Back (closes dropdowns and sub pages, then the menu) |
| LB / RB | Switch tabs |

## Building

1. Open `module.sln` in Visual Studio 2010 with the Xbox 360 XDK installed.
2. Select the `Release Retail | Xbox 360` configuration and build.
3. The post-build step runs `xextool` and writes the finished `.xex` into `module/Release Retail`.
4. Load the `.xex` into the running BO2 Multiplayer title with your usual plugin loader.

## Adding your own option

1. Add a variable for it at the top of `Functions.cpp` and an `extern` for it in `Functions.h`:

   ```cpp
   bool bMyOption = false;
   ```

2. Write a function that takes the new value:

   ```cpp
   void MyOption(bool value)
   {
       // Do something with value
   }
   ```

3. Add one line to the tab you want in `Menu.cpp`:

   ```cpp
   AddToggleOption("My Option", &bMyOption, MyOption);
   ```

Other items work the same way:

```cpp
AddSection("My Section");
AddToggleOption("Toggle", &bValue);
AddSliderBar("Slider", &fValue, 0.0f, 100.0f, 1.0f, 0);
AddDropdown("Dropdown", &iIndex, kOptions, 3);
AddButton("Button", &MyFunction);
```
