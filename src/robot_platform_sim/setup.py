from setuptools import find_packages, setup

package_name = 'robot_platform_sim'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='zhangheng',
    maintainer_email='511097824@qq.com',
    description='Simulation executor and status adapter for robot platform',
    license='Apache-2.0',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'simple_executor_node = robot_platform_sim.simple_executor_node:main',
            'status_adapter_node = robot_platform_sim.status_adapter_node:main',
        ],
    },
)
