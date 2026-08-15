/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

package main

import (
	"errors"
	"fmt"
	"io"
	"os"
	"strings"

	"github.com/chzyer/readline"
)

var errQuit = errors.New("quit")

type Command struct {
	Name string
	Args []string
}

type CommandHandler func(args []string) error

type CommandDef struct {
	Name        string
	Description string
	Handler     CommandHandler
}

type REPL struct {
	commands []CommandDef
}

func NewREPL() *REPL {
	r := &REPL{}

	r.commands = []CommandDef{
		{
			Name:        "help",
			Description: "Show available commands",
			Handler:     r.cmdHelp,
		},
        {
			Name:        "add",
			Description: "Add a new partition for the MTD device",
			Handler:     r.cmdAddSpecific,
		},
        {
			Name:        "add",
			Description: "Add a set of new partitions for the MTD device from a context table",
			Handler:     r.cmdDispatchPartitionTable,
		},
		{
			Name:        "delete_all",
			Description: "Delete ALL active partitions of the MTD device",
			Handler:     r.cmdDeleteAll,
		},
        {
			Name:        "delete",
			Description: "Delete an active partition of the MTD device",
			Handler:     r.cmdDeleteSpecific,
		},
		{
			Name:        "status",
			Description: "Show in-progress partition table",
			Handler:     r.cmdPartitionTableStatus,
		},
		{
			Name:        "reset",
			Description: "Reset in-progress partition table",
			Handler:     r.cmdPartitionTableReset,
		},
		{
			Name:        "exit",
			Description: "Exit the REPL",
			Handler:     r.cmdExit,
		},
		{
			Name:        "quit",
			Description: "Exit the REPL",
			Handler:     r.cmdExit,
		},
	}

	return r
}

func (r *REPL) Run() error {
	rl, err := readline.NewEx(&readline.Config{
		Prompt:          "mtdpartctl> ",
		InterruptPrompt: "^C",
		EOFPrompt:       "exit",
	})
	if err != nil {
		return fmt.Errorf("initialize readline: %w", err)
	}
	defer rl.Close()

	fmt.Println("MyApp REPL")
	fmt.Println("Type 'help' for available commands.")

	for {
		line, err := rl.Readline()

		switch err {
		case nil:
			// Continue.

		case readline.ErrInterrupt:
			// Ctrl-C.
			continue

		case io.EOF:
			// Ctrl-D.
			fmt.Println()
			return nil

		default:
			return fmt.Errorf("read input: %w", err)
		}

		line = strings.TrimSpace(line)

		if line == "" {
			continue
		}

		cmd, err := parseCommand(line)
		if err != nil {
			fmt.Fprintln(os.Stderr, "error:", err)
			continue
		}

		err = r.execute(cmd)

		if errors.Is(err, errQuit) {
			return nil
		}

		if err != nil {
			fmt.Fprintln(os.Stderr, "error:", err)
		}
	}
}

func parseCommand(line string) (Command, error) {
	fields := strings.Fields(line)

	if len(fields) == 0 {
		return Command{}, nil
	}

	return Command{
		Name: fields[0],
		Args: fields[1:],
	}, nil
}

func (r *REPL) execute(cmd Command) error {
	for _, def := range r.commands {
		if def.Name == cmd.Name {
			return def.Handler(cmd.Args)
		}
	}

	return fmt.Errorf("unknown command %q", cmd.Name)
}

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

func (r *REPL) cmdHelp(args []string) error {
	fmt.Println("Available commands:")

	for _, cmd := range r.commands {
		fmt.Printf("  %-12s %s\n", cmd.Name, cmd.Description)
	}

	return nil
}

func (r *REPL) cmdAddSpecific(args []string) error {
	fmt.Println(strings.Join(args, " "))
	return nil
}

func (r *REPL) cmdDeleteSpecific(args []string) error {
	fmt.Println(strings.Join(args, " "))
	return nil
}

func (r *REPL) cmdDeleteAll(args []string) error {
	fmt.Println(strings.Join(args, " "))
	return nil
}

func (r *REPL) cmdDispatchPartitionTable(args []string) error {
	fmt.Println(strings.Join(args, " "))
	return nil
}

func (r *REPL) cmdPartitionTableStatus(args []string) error {
	fmt.Println("status: OK")
	return nil
}

func (r *REPL) cmdPartitionTableReset(args []string) error {
	fmt.Printf("Reset\n")
	return nil
}

func (r *REPL) cmdExit(args []string) error {
	return errQuit
}

func main() {
	repl := NewREPL()

	if err := repl.Run(); err != nil {
		fmt.Fprintln(os.Stderr, "error:", err)
		os.Exit(1)
	}
}
