#!/usr/bin/env python
# -*- coding: utf-8 -*-

#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function

import sys, os
import argparse

from appnode import BuildNode, RemoteBuildNode, RunNode, RemoteRunNode, NodeInfo

import pyzmq as zmq

class AlloRunner():
    def __init__(self, nodes, project_src, verbose = True, debug = False):
        self.current_index = 0
        self.buffer_len = 256
        self.chase = True
        self.verbose = verbose
        self.debug = debug
        self.project_src = project_src

        self.log_text = ''
        if self.verbose:
            self.log('Started...\n')
        if nodes and len(nodes) > 0:
            self.nodes = nodes
            if self.verbose:
                for node in nodes:
                    try:
                        name = node.name
                    except:
                        name = node.hostname
                    self.log("----- Node:%s -----"%name)
                    self.log("Remote: " + str(node.remote))
                    self.log("Hostname:" + node.hostname)

        #self.item_color = { "none": 7, "error" : 1, "done" : 2, "working" : 4}

        self.displays = ["STDOUT", "STDERR"]
        self.current_display = 0
        self.world = {}
        pass

    def log(self, text):
        if not text == '':
            if not text[-1] == '\n':
                text = text + '\n'
            self.log_text += text
            print(text, end = '')

    def start(self):
        if (self._run_builders()) :
            self._start_runners()
        else:
            self.log("Building failed.")

    def _run_builders(self):
        self.builders = []
        self.stdoutbuf  = ['' for i in self.nodes]
        self.stderrbuf = ['' for i in self.nodes]
        for i, node in enumerate(self.nodes):

            builder = node
            from os.path import expanduser
            home = expanduser("~")
            cwd = os.getcwd()
            if builder.remote:
                cwd = builder.scratch_path + '/' + user_prefix + '/' + node.hostname + '/' + cwd[cwd.rfind('/')+ 1 :]
#                if cwd[:len(home)] == home:
#                    cwd = cwd[len(home) + 1:]
                print(cwd)

#            builder.configure(**configuration)
            builder.set_debug(self.debug)
            self.builders.append(builder)
            builder.build()
        done = True
        self.log("Start build")
        for b in self.builders:
            if not b.is_done():
                done = False
                break
        while not done:
            for i in range(len(self.builders)):
                std, err = self.builders[i].read_messages()
                self.stdoutbuf[i] += std
                self.stderrbuf[i] += err

                if std:
                    self.log(std)
                if err:
                    self.log(err)

#                num_lines = console_output.count('\n') + 1
#                if num_lines >= self.buffer_len:
#                    lines = console_output.split('\n')
#                    console_output = '\n'.join(lines[len(lines) - self.buffer_len:])
#
#                    num_lines = console_output.count('\n')

            done = True
            for builder in self.builders:
                if not builder.is_done():
                    done = False
                    break

        self.log(builder.stdout)
        self.log(builder.stderr)
        self.log("Building done  ---- ")

        return True
    
    
    def _bootstrap(self):
        
        root_found = False
        for runner in self.runners:
            if runner.rank == 0:
                root_found = True
                break
        
        if not root_found:
            raise ValueError("No root node.")
        
        port = "5556"
        context = zmq.Context()
        socket = context.socket(zmq.PAIR)
        while not socket.bind("tcp://*:%s" % port):
            port = port + 1
        
        socket.send(b"Server message to client3")
        msg = socket.recv()

    def _start_runners(self):
        self.stdoutbuf  = []
        self.stderrbuf = []
        self.runners = []
        rank = 0
        
        self._bootstrap()

        for i, node in enumerate(self.nodes):
            for app in node.get_products():
                for deploy_host in node.deploy_to:
                    self.stdoutbuf.append('')
                    self.stderrbuf.append('')
                    if node.remote:
                        runner = RemoteRunNode(deploy_host + '-' + app, deploy_host, rank)
                    else:
                        runner = RunNode(deploy_host + '-' + app, rank)
                    bin_path = 'bin\\' + app
                    runner.configure(node.project_dir, bin_path)
                    self.runners.append(runner)
                    rank = rank + 1
        done = False
        
        self._bootstrap()
        # Run all other nodes
        for runner in self.runners:
            if not runner.rank == 0:
                runner.run()

        while not done:
            for i in range(len(self.runners)):
                std, err = self.runners[i].read_messages()
                self.stdoutbuf[i] += std
                self.stderrbuf[i] += err

                if std:
                    self.log(std)
                if err:
                    self.log(err)

            done = True
            for b in self.runners:
                if not b.is_done():
                    done = False
                    break

        for runner in self.runners:
            self.log(runner.stdout)
            self.log(runner.stderr)
            runner.thread.join(0.1)

        self.log("Running done.")


    def stop(self):
        for runner in self.runners:
            runner.terminate()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Build and Run AlloSphere apps.')
    parser.add_argument('source', type=str, nargs="*",
                       help='source file or directory to build',
                       default='')
    parser.add_argument('-l', '--local', help='Force local build.', action="store_true")
    parser.add_argument('-a', '--allosphere', help='Force Allosphere build.', action="store_true")
    parser.add_argument('-v', '--verbose', help='Verbose output.', action="store_true")
    parser.add_argument('-d', '--debug', help='Build debug version.', action="store_true")

    args = parser.parse_args()
    project_src = args.source

    import socket
    hostname = socket.gethostname()

    cur_dir = os.getcwd()
    if os.name == 'nt':
        project_name = cur_dir[cur_dir.rindex('\\') + 1:]
    else:
        project_name = cur_dir[cur_dir.rindex('/') + 1:]


    if (hostname == 'audio.10g' and not args.local) or (args.allosphere):
        from allosphere import nodes
    else:
        from local import nodes

    if args.allosphere:
        user_prefix = 'andres'
        for node in nodes:
            scratch_path = node.scratch_path
            exclude_dirs = ['"CMakeCache.txt"', '"build"', '".git"', '"AlloSystem/.git"', '"AlloSystem/CMakeFiles"','"*/CMakeFiles"', '"*/AlloSystem-build"']
            rsync_command = 'rsync -arv -p --delete --group=users ' + "--exclude " + ' --exclude '.join(exclude_dirs) + " "
            rsync_command += cur_dir + ' '
            rsync_command += "sphere@%s:"%node.hostname + scratch_path + "/" + user_prefix + "/" + node.hostname
            print("syncing:  " + rsync_command)
            os.system(rsync_command)

    import traceback

    if len(nodes) > 0:
        app = AlloRunner(nodes, project_src, args.verbose, args.debug)
        try:
            app.start()
        except:
            print("Python exception ---")
            traceback.print_exc()
            print(sys.exc_info()[0])
#        if not app.log_text == '':
#            print('Output log: -------------\n' + app.log_text)
    else:
        print("No nodes. Aborting.")
