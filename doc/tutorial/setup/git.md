---
title: Cloning the Repository
description: This tutorial shows how to clone the qiotoolkit repository.
topic: article
uid: qiotoolkit.tutorial.setup.git
---

Cloning the Repository
======================

> [!NOTE]
> These instructions are for \*sh/linux.

1) To get a current copy of qiotoolkit, head on over to its and
   click the \[Clone\] button in the top right corner.

2) Select the mode of download (I recommend SSH, for which you might need to
   [upload your SSH public key](ssh-setup.md)).

3) Copy the repository handle and run `git clone` in your CLI:
   
4) Check the contents of the `qiotoolkit` folder, which was just created.
   ```bash
   $ ls qiotoolkit
     azure-pipelines.yml  cpp  doc  ...
   ```

5) You are free to move this folder to a suitable place and/or rename it.
   ```bash
   $ mv qiotoolkit ~/work/qiotoolkit
   ```

_After cloning, continue by [setting up the prerequisites](prerequisites.md)._
