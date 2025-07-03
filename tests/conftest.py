import os
import sys
import pytest
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