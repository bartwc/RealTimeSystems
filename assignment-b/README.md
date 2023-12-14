# Real-time Systems Lab - Assignment B: Programming a Synthesizer

In this assignment, you will be improving the organization of processing tasks in a digital synthesizer. The synthesizer runs on a combination of the STM32F4DISCOVERY board used in assignment A and a peripherals board containing switches and encoders used to interact with the synthesizer. Provided with a set of tasks that the synthesizer needs to complete, you will determine each task's timing constraints and implement the best way to organize and schedule those tasks within the Zephyr RTOS.

> If you have not installed Zephyr and verified the setup yet, go through the lab setup [here](https://cese.pages.ewi.tudelft.nl/real-time-systems/lab_setup.html).

> [Link to Assignment B](https://cese.pages.ewi.tudelft.nl/real-time-systems/assignment-b-intro.html)


## Building the Application
1\. Navigate to the `zephyrproject` directory on your machine. Activate the Python virtual environment in your terminal. This will give you access to the `west` compile commands needed to build and flash the application onto the development board.

For Linux/MacOS:
```
$ source ~/zephyrproject/.venv/bin/activate
```

For Windows:
```
$ %HOMEPATH%\zephyrproject\.venv\Scripts\activate.bat 
```

If the command completed successfully, you should see a message `(.venv)` prepended to your normal terminal prompt.

2\. Next, you will use the `west` command to build and flash the target application. When building the project for the first time, you need to specify to the `west` infrastructure which development board to target and where to find the `CMakeLists.txt` file specifying the source code to compile. This is done with the following command.

```
(.venv) $ west build -b stm32f4_disco <path_to_assignment-b_repository>
```

Subsequently, you can flash the compiled binary to the develpment board using the following command:

```
(.venv) $ west flash
```

> Note: you only need to execute the `west build` command when building the project for the first time. If you make simple modifications to the code, you can simply run `west flash` and the tool will automatically check for differences in the source code and recompile, if necessary.

## Submission Information

***Deadline: Saturday 20th of January 23:59 (week 8)***

The assignment is submitted by pushing to the `main` branch of this repository.
The last commit before the deadline will be considered your submission.
We recommend frequently committing in order to prevent losing progression.
The report part of this assignment should be submitted as a `.pdf` (LaTeX or Markdown). The filename should be formatted as:
`assignment_b_report_group_X.pdf` where X is your group number. Inside the report you should specify your names and student numbers.
Please put any figures you use in the resources directory.

>Following your assignment's submission, the TAs might conduct oral checks about the assignment. For example, you might be asked to change parts of the code or explain the report. There will be a follow up announcement with more information.