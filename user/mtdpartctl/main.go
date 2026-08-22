/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

package main

import (
	"errors"
	"flag"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
	"unsafe"

	"github.com/chzyer/readline"
	"github.com/mattn/go-shellwords"
	"gopkg.in/yaml.v3"
)

/*
#include "dev.h"
#include <stdlib.h>
*/
import "C"

var errQuit = errors.New("quit")

type Partition struct {
	Name   string `yaml:"name"`
	Offset string `yaml:"offset"`
	Length string `yaml:"length"`
	Flags  string `yaml:"flags"`
}

type Config []Partition

type Command struct {
	Name string
	Args []string
}

type CommandHandler func(file *os.File, args []string) error

type CommandDef struct {
	Name          string
	Description   string
	Handler       CommandHandler
	ArgumentsHelp string
	Examples      []string
}

type REPL struct {
	commands         []CommandDef
	mtdpartctlFile   *os.File
	recipePartsCount uint
}

func NewREPL(mtdpartctlFile *os.File) *REPL {
	r := &REPL{}
	r.mtdpartctlFile = mtdpartctlFile
	r.recipePartsCount = 0

	r.commands = []CommandDef{
		{
			Name:          "help",
			Description:   "Show available commands and print arguments for a specific command",
			Handler:       r.cmdHelp,
			ArgumentsHelp: "<command_name>",
			Examples:      []string{"help add_imm"},
		},
		{
			Name:          "add_imm",
			Description:   "Add a new partition for the MTD device",
			Handler:       r.cmdAddImmediate,
			ArgumentsHelp: "<offset> <length> <name string>",
			Examples:      []string{"add_imm 0 0x10000 \"test 0\""},
		},
		{
			Name:          "dispatch_recipe",
			Description:   "Add a set of new partitions for the MTD device from a recipe table",
			Handler:       r.cmdDispatchRecipe,
			ArgumentsHelp: "none",
			Examples:      []string{"dispatch_recipe"},
		},
		{
			Name:          "add_recipe_part",
			Description:   "Add a recipe part for the MTD device",
			Handler:       r.cmdAddRecipePart,
			ArgumentsHelp: "<offset> <length> <name string> <flags>",
			Examples: []string{"add_recipe_part 0 0x10000 \"test 0\" rw,powerup_lock",
				"add_recipe_part 131072 131072 \"test 1\""},
		},
		{
			Name:          "del_recipe_part",
			Description:   "Delete a recipe part by its index (starting at 0) for the MTD device",
			Handler:       r.cmdDeleteRecipePart,
			ArgumentsHelp: "<index>",
			Examples:      []string{"del_recipe_part 0"},
		},
		{
			Name:          "delete_all",
			Description:   "Delete ALL (present) partitions of the MTD device",
			Handler:       r.cmdDeleteAll,
			ArgumentsHelp: "none",
			Examples:      []string{"delete_all"},
		},
		{
			Name:          "delete",
			Description:   "Delete an active partition of the MTD device by its index (starting at 0)",
			Handler:       r.cmdDeleteImmediate,
			ArgumentsHelp: "<index>",
			Examples:      []string{"delete 0"},
		},
		{
			Name:          "status",
			Description:   "Show in-progress recipe table",
			Handler:       r.cmdRecipeStatus,
			ArgumentsHelp: "none",
			Examples:      []string{"status"},
		},
		{
			Name:          "reset",
			Description:   "Reset in-progress recipe",
			Handler:       r.cmdRecipeReset,
			ArgumentsHelp: "none",
			Examples:      []string{"reset"},
		},
		{
			Name:          "exit",
			Description:   "Exit the REPL",
			Handler:       r.cmdExit,
			ArgumentsHelp: "none",
			Examples:      []string{"exit"},
		},
		{
			Name:          "quit",
			Description:   "Exit the REPL",
			Handler:       r.cmdExit,
			ArgumentsHelp: "none",
			Examples:      []string{"quit"},
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

	fmt.Println("mtdpartctl REPL")
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

		err = r.execute(r.mtdpartctlFile, cmd)

		if errors.Is(err, errQuit) {
			return nil
		}

		if err != nil {
			fmt.Fprintln(os.Stderr, "error:", err)
		}
	}
}

func parseCommand(line string) (Command, error) {
	fields, err := shellwords.Parse(line)
	if err != nil {
		return Command{}, fmt.Errorf("invalid command arguments")
	}

	if len(fields) == 0 {
		return Command{}, nil
	}

	return Command{
		Name: fields[0],
		Args: fields[1:],
	}, nil
}

func verifyCount(args []string, count uint) error {
	if uint(len(args)) != count {
		return fmt.Errorf("invalid argument count")
	}

	return nil
}

func (r *REPL) execute(file *os.File, cmd Command) error {
	for _, def := range r.commands {
		if def.Name == cmd.Name {
			return def.Handler(file, cmd.Args)
		}
	}

	return fmt.Errorf("unknown command %q", cmd.Name)
}

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

func (r *REPL) cmdHelp(file *os.File, args []string) error {
	if err := verifyCount(args, 1); err == nil {
		for _, cmd := range r.commands {
			if cmd.Name == args[0] {
				fmt.Printf("  %-16s %s\n", cmd.Name, cmd.Description)
				fmt.Printf("  arguments: %s\n", cmd.ArgumentsHelp)
				fmt.Printf("  usage examples:\n")
				for _, example := range cmd.Examples {
					fmt.Printf("    %s\n", example)
				}
				return nil
			}
		}
		return fmt.Errorf("invalid argument count\n")
	}

	if err := verifyCount(args, 0); err != nil {
		return fmt.Errorf("invalid argument count\n")
	}

	fmt.Println("Available commands:")

	for _, cmd := range r.commands {
		fmt.Printf("  %-16s %s\n", cmd.Name, cmd.Description)
	}

	return nil
}

func (r *REPL) cmdAddImmediate(file *os.File, args []string) error {
	if err := verifyCount(args, 3); err != nil {
		return err
	}

	offset, err := strconv.ParseUint(args[0], 0, 64)
	if err != nil {
		return fmt.Errorf("invalid argument: %s\n", err)
	}

	length, err := strconv.ParseUint(args[1], 0, 64)
	if err != nil {
		return fmt.Errorf("invalid argument: %s\n", err)
	}

	cs := C.CString(args[2])
	defer C.free(unsafe.Pointer(cs))

	rc := C.__go_mtdpartctl_add_new_partition(C.int(r.mtdpartctlFile.Fd()),
		C.uint64_t(offset), C.uint64_t(length), cs)
	if rc < 0 {
		return fmt.Errorf("ioctl result: %d\n", rc)
	}

	r.recipePartsCount++

	return nil
}

// returns (rw, lockup_enabled, error)
func parseRecipeFlags(input string) (bool, bool, error) {
	flagTrimmed := strings.TrimSpace(input)

	flags := map[string]bool{
		"ro":           false,
		"powerup_lock": false,
	}

	if flagTrimmed == "" {
		return !flags["ro"], flags["lockup_enabled"], nil
	}

	flagsSplitted := strings.Split(input, ",")

	for _, flag := range flagsSplitted {
		flag = strings.TrimSpace(flag)

		if _, exists := flags[flag]; !exists {
			return false, false, fmt.Errorf("invalid flag: %q\n", flag)
		}

		flags[flag] = true
	}

	return !flags["ro"], flags["lockup_enabled"], nil
}

func addRecipePart(fd int, offsetStr string, lengthStr string, name string, flags string) error {
	writable, lockup_enabled, err := parseRecipeFlags(flags)
	if err != nil {
		return fmt.Errorf("%s\n", err)
	}

	offset, err := strconv.ParseUint(offsetStr, 0, 64)
	if err != nil {
		return fmt.Errorf("invalid argument: %s\n", err)
	}

	length, err := strconv.ParseUint(lengthStr, 0, 64)
	if err != nil {
		return fmt.Errorf("invalid argument: %s\n", err)
	}

	cs := C.CString(name)
	defer C.free(unsafe.Pointer(cs))

	rc := C.__go_mtdpartctl_recipe_add_part(C.int(fd),
		C.uint64_t(offset), C.uint64_t(length), cs, C.bool(writable), C.bool(lockup_enabled))
	if rc < 0 {
		return fmt.Errorf("ioctl result: %d\n", rc)
	}

	return nil
}

func (r *REPL) cmdAddRecipePart(file *os.File, args []string) error {
	flags := ""

	if err := verifyCount(args, 4); err == nil {
		flags = args[3]
	} else {
		if err := verifyCount(args, 3); err != nil {
			return err
		}
	}

	if err := addRecipePart(int(r.mtdpartctlFile.Fd()), args[0], args[1], args[2], flags); err != nil {
		return err
	}

	r.recipePartsCount++

	return nil
}

func (r *REPL) cmdDeleteRecipePart(file *os.File, args []string) error {
	if err := verifyCount(args, 1); err != nil {
		return err
	}

	index, err := strconv.ParseUint(args[0], 10, 0)
	if err != nil {
		return fmt.Errorf("invalid argument: %s\n", err)
	}

	rc := C.mtdpartctl_recipe_del_part(C.int(r.mtdpartctlFile.Fd()), C.size_t(index))
	if rc < 0 {
		return fmt.Errorf("ioctl result: %d\n", rc)
	}

	r.recipePartsCount--
	return nil
}

func (r *REPL) cmdDeleteImmediate(file *os.File, args []string) error {
	if err := verifyCount(args, 1); err != nil {
		return err
	}

	index, err := strconv.ParseUint(args[0], 10, 32)
	if err != nil {
		return fmt.Errorf("invalid argument: %s\n", err)
	}

	rc := C.mtdpartctl_delete_partition(C.int(r.mtdpartctlFile.Fd()), C.uint32_t(index))
	if rc < 0 {
		return fmt.Errorf("ioctl result: %d\n", rc)
	}

	return nil
}

func (r *REPL) cmdDeleteAll(file *os.File, args []string) error {
	if err := verifyCount(args, 0); err != nil {
		return err
	}

	rc := C.mtdpartctl_delete_mtd_partitions(C.int(r.mtdpartctlFile.Fd()))
	if rc < 0 {
		return fmt.Errorf("ioctl result: %d\n", rc)
	}
	return nil
}

func (r *REPL) cmdDispatchRecipe(file *os.File, args []string) error {
	if err := verifyCount(args, 0); err != nil {
		return err
	}

	rc := C.mtdpartctl_recipe_create_partitions(C.int(r.mtdpartctlFile.Fd()))
	if rc < 0 {
		return fmt.Errorf("ioctl result: %d\n", rc)
	}

	return nil
}

func (r *REPL) cmdRecipeStatus(file *os.File, args []string) error {
	if err := verifyCount(args, 0); err != nil {
		return err
	}

	if r.recipePartsCount == 0 {
		fmt.Println("Empty table")
		return nil
	}

	rc := C.mtdpartctl_recipe_print_parts(C.int(r.mtdpartctlFile.Fd()), 255)
	if rc < 0 {
		return fmt.Errorf("ioctl result: %d\n", rc)
	}

	return nil
}

func (r *REPL) cmdRecipeReset(file *os.File, args []string) error {
	if err := verifyCount(args, 0); err != nil {
		return err
	}

	rc := C.mtdpartctl_recipe_restart(C.int(r.mtdpartctlFile.Fd()))
	if rc < 0 {
		return fmt.Errorf("ioctl result: %d\n", rc)
	}

	r.recipePartsCount = 0

	return nil
}

func (r *REPL) cmdExit(file *os.File, args []string) error {
	return errQuit
}

func openDevice(devicePath string) (*os.File, C.struct_mtdpartctl_info, error) {
	mtdpartctl_info := C.struct_mtdpartctl_info{}

	f, err := os.OpenFile(devicePath, os.O_RDWR, 0)
	if err != nil {
		return nil, mtdpartctl_info, fmt.Errorf("open %q: %v\n", devicePath, err)
	}
	fd := f.Fd() // uintptr

	// Try to do basic ioctl just to see if it's an mtdpartctl device
	rc := C.mtdpartctl_get_info(C.int(fd), &mtdpartctl_info)
	if rc < 0 {
		return nil, mtdpartctl_info, fmt.Errorf("mtdpartctl_info ioctl on %q result: %d\n", devicePath, rc)
	}
	return f, mtdpartctl_info, nil
}

func humanBytes(n uint64) string {
	const unit = 1024

	if n < unit {
		return fmt.Sprintf("%d B", n)
	}

	div, exp := uint64(unit), 0
	for n >= div*unit && exp < 4 {
		div *= unit
		exp++
	}

	return fmt.Sprintf("%.1f %ciB", float64(n)/float64(div), "KMGT"[exp])
}

func useConfigFile(mtdpartctlFile *os.File, configPath string) error {
	data, err := os.ReadFile(configPath)
	if err != nil {
		return fmt.Errorf("read error: %v\n", err)
	}

	var partitions []Partition
	if err := yaml.Unmarshal(data, &partitions); err != nil {
		return fmt.Errorf("unmarshal error: %v\n", err)
	}

	for _, partition := range partitions {
		if err := addRecipePart(int(mtdpartctlFile.Fd()), partition.Offset,
			partition.Length, partition.Name, partition.Flags); err != nil {
			return err
		}
	}

	rc := C.mtdpartctl_recipe_create_partitions(C.int(mtdpartctlFile.Fd()))
	if rc < 0 {
		return fmt.Errorf("ioctl result: %d\n", rc)
	}

	return nil
}

func main() {
	printInfoOnly := flag.Bool("info-only", false, "print device information and exit")
	printVerbose := flag.Bool("verbose", false, "enable verbose mode")
	resetProxy := flag.Bool("reset-proxy", false, "reset the proxy MTD")
	configFile := flag.String("config", "", "YAML configuration file")

	flag.Parse()

	if flag.NArg() != 1 {
		fmt.Fprintf(os.Stderr, "usage: %s <device>\n", os.Args[0])
		os.Exit(2)
	}

	devicePath := flag.Arg(0)
	f, info_struct, err := openDevice(devicePath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "fatal error: %s", err)
		os.Exit(1)
	}
	defer f.Close()

	if *printVerbose {
		fmt.Fprintf(os.Stderr, "Succesfully opened %s\n", devicePath)
	}

	if *printInfoOnly || *printVerbose {
		fmt.Printf("MTD backing index: %d\n", info_struct.backend_mtd_index)
		fmt.Printf("current MTD proxy index: %d\n", info_struct.proxy_mtd_index)
		fmt.Printf("Erase sector size: %d bytes (%s)\n", info_struct.erase_sector_size, humanBytes(uint64(info_struct.erase_sector_size)))
		fmt.Printf("MTD backing size: %d bytes (%s)\n", info_struct.backend_mtd_size, humanBytes(uint64(info_struct.backend_mtd_size)))
		if *printInfoOnly {
			os.Exit(0)
		}
	}

	if *resetProxy {
		rc := C.mtdpartctl_delete_mtd_partitions(C.int(f.Fd()))
		if rc < 0 {
			fmt.Fprintf(os.Stderr, "ioctl result: %d\n", rc)
			os.Exit(1)
		}
		os.Exit(0)
	}

	if *configFile != "" {
		if err := useConfigFile(f, *configFile); err != nil {
			fmt.Fprintf(os.Stderr, "error: %v", err)
			os.Exit(1)
		}
		os.Exit(0)
	}
	repl := NewREPL(f)

	if err := repl.Run(); err != nil {
		fmt.Fprintln(os.Stderr, "error:", err)
		os.Exit(1)
	}
}
