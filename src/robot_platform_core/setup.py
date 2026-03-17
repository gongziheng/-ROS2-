from setuptools import find_packages, setup

package_name = 'robot_platform_core'

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
    maintainer_email='zhangheng@todo.todo',
    description='TODO: Package description',
    license='TODO: License declaration',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'fake_robot_node=robot_platform_core.fake_robot_node:main',
            'task_manager_node=robot_platform_core.task_manager_node:main',
            'alert_monitor_node=robot_platform_core.alert_monitor_node:main',
        ],
    },
)
