# QucsatorRF v1.0.6

* Fixed bugs related to VCD files conversion #36

# QucsatorRF v1.0.5

* Fixed bugs related to data files converter #34

# QucsatorRF v1.0.4

* Fix datasets conversion #33

# QucsatorRF v1.0.3

* Fixed reading the netlist from STDIN #24

# QucsatorRF v1.0.2

* Added simplified TRAN models for mscorner, msmbend, and bondwire #19
* Fixed equations in mscouple #20
* Fixed equations in rectline #21

# QucsatorRF v1.0.1

* Show warning instead of error on wrong property name and ignore this property #14
* Add `-ln` command line switch for converter. This switch allows to define Qucs library name #11
* Fixed Qucsconv error if data variable has SPICE notation #4
* Fixed build on MacOS #3

# QucsatorRF v1.0.0

## New features

* Added advanced radial stub model by @michal777
* Fixed S-matrix transform by @thliebig
* Build system switched to CMake
* ADMS dependency made optional
* Fixed `No .END directive` warning in converter
