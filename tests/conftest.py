import os
import sys
import pytest

# On Windows, with Python 3.8+, we need to add the vcpkg bin directory
# to the DLL search path *before* osgeo.gdal is imported. This ensures that
# the Python bindings load the same GDAL DLLs that the C++ plugin was
# linked against.
if sys.platform == "win32" and sys.version_info >= (3, 8):
    vcpkg_gdal_bin_dir = os.getenv("VCPKG_GDAL_BIN_DIR")
    if vcpkg_gdal_bin_dir and os.path.isdir(vcpkg_gdal_bin_dir):
        print(f"Adding to DLL search path: {vcpkg_gdal_bin_dir}")
        os.add_dll_directory(vcpkg_gdal_bin_dir)
    else:
        # This is not a fatal error, but a warning, as the PATH might still be correct.
        print("Warning: VCPKG_GDAL_BIN_DIR is not set or not a valid directory.")

# Now that the DLL path is set, we can safely import gdal.
from osgeo import gdal

@pytest.fixture(scope="session", autouse=True)
def check_gdal_driver_path():
    """Ensure the GDAL_DRIVER_PATH is set for the tests."""
    driver_path = os.environ.get('GDAL_DRIVER_PATH')
    if not driver_path:
        pytest.fail("The GDAL_DRIVER_PATH environment variable is not set.")
    
    print(f"GDAL_DRIVER_PATH is set to: {driver_path}")
    
    if not os.path.isdir(driver_path):
        pytest.fail(f"The directory specified by GDAL_DRIVER_PATH does not exist: {driver_path}")


@pytest.fixture(scope="session", autouse=True)
def register_gdal_driver():
    """Fixture to ensure the custom driver is registered before tests run."""
    gdal.AllRegister()