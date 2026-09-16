package commands

import (
	"embed"
	"fmt"
	"log"
	"path"
	"strings"
	"time"

	lua "github.com/yuin/gopher-lua"
)

//go:embed lua/*.lua
var luaScripts embed.FS

var luaState *lua.LState

func InitLua() {
	luaState = lua.NewState()

	luaState.PreloadModule("cero", ceroModule)

	entries, err := luaScripts.ReadDir("lua")
	if err != nil {
		log.Fatalf("lua: read embedded scripts: %v", err)
	}

	for _, e := range entries {
		if e.IsDir() || !strings.HasSuffix(e.Name(), ".lua") {
			continue
		}

		file := path.Join("lua", e.Name())

		data, err := luaScripts.ReadFile(file)
		if err != nil {
			log.Printf("lua: %s: %v", e.Name(), err)
			continue
		}

		if err := luaState.DoString(string(data)); err != nil {
			log.Printf("lua: %s: %v", e.Name(), err)
		}
	}
}

func ceroModule(L *lua.LState) int {
	mod := L.SetFuncs(L.NewTable(), map[string]lua.LGFunction{
		"out":      luaOut,
		"query":    luaQuery,
		"queryrow": luaQueryRow,
		"exec":     luaExec,
		"finduser": luaFindUser,
		"uptime":   luaUptime,
		"hub":      luaHub,
	})

	mod.RawSetString("define", L.NewFunction(func(ls *lua.LState) int {
		name := ls.CheckString(1)
		desc := ls.CheckString(2)
		fn := ls.CheckFunction(3)

		ls.SetGlobal("cmd:"+name, fn)

		Register(Command{
			Name: name,
			Desc: desc,

			Run: func(args []string) {
				ls.Push(fn)

				for _, a := range args {
					ls.Push(lua.LString(a))
				}

				if err := ls.PCall(len(args), 0, nil); err != nil {
					out("lua error: %v\n", err)
				}

				ls.SetTop(0)
			},
		})

		return 0
	}))

	L.Push(mod)
	return 1
}

func luaOut(L *lua.LState) int {
	top := L.GetTop()

	parts := make([]string, 0, top)

	for i := 1; i <= top; i++ {
		parts = append(
			parts,
			L.ToStringMeta(L.Get(i)).String(),
		)
	}

	out("%s\n", strings.Join(parts, " "))

	return 0
}

func luaQuery(L *lua.LState) int {
	sqlText := L.CheckString(1)
	args := luaToArgs(L, 2)

	rows, err := deps.DB.Query(sqlText, args...)
	if err != nil {
		L.Push(lua.LNil)
		L.Push(lua.LString(err.Error()))
		return 2
	}

	defer rows.Close()

	result := L.NewTable()

	cols, _ := rows.Columns()

	for rows.Next() {
		vals := make([]any, len(cols))
		ptrs := make([]any, len(cols))

		for i := range vals {
			ptrs[i] = &vals[i]
		}

		if rows.Scan(ptrs...) != nil {
			continue
		}

		row := L.NewTable()

		for i, c := range cols {
			row.RawSetString(
				c,
				goToLua(L, vals[i]),
			)
		}

		result.Append(row)
	}

	L.Push(result)

	return 1
}

func luaQueryRow(L *lua.LState) int {
	sqlText := L.CheckString(1)
	args := luaToArgs(L, 2)

	rows, err := deps.DB.Query(sqlText, args...)
	if err != nil {
		L.Push(lua.LNil)
		L.Push(lua.LString(err.Error()))
		return 2
	}

	defer rows.Close()

	if !rows.Next() {
		L.Push(lua.LNil)
		return 1
	}

	cols, _ := rows.Columns()

	vals := make([]any, len(cols))
	ptrs := make([]any, len(cols))

	for i := range vals {
		ptrs[i] = &vals[i]
	}

	if rows.Scan(ptrs...) != nil {
		L.Push(lua.LNil)
		return 1
	}

	row := L.NewTable()

	for i, c := range cols {
		row.RawSetString(
			c,
			goToLua(L, vals[i]),
		)
	}

	L.Push(row)

	return 1
}

func luaExec(L *lua.LState) int {
	sqlText := L.CheckString(1)
	args := luaToArgs(L, 2)

	if _, err := deps.DB.Exec(sqlText, args...); err != nil {
		L.Push(lua.LString(err.Error()))
		return 1
	}

	L.Push(lua.LNil)

	return 1
}

func luaFindUser(L *lua.LState) int {
	u := findUser(L.CheckString(1))

	if u == nil {
		L.Push(lua.LNil)
		return 1
	}

	t := L.NewTable()

	t.RawSetString(
		"uuid",
		lua.LString(u.UUID),
	)

	t.RawSetString(
		"username",
		lua.LString(u.Username),
	)

	L.Push(t)

	return 1
}

func luaUptime(L *lua.LState) int {
	L.Push(
		lua.LString(
			time.Since(startedAt).
				Round(time.Second).
				String(),
		),
	)

	return 1
}

func luaHub(L *lua.LState) int {
	if deps.Hub == nil {
		L.Push(lua.LNil)
		return 1
	}

	h := L.NewTable()

	h.RawSetString(
		"online",
		lua.LNumber(deps.Hub.TotalOnline()),
	)

	h.RawSetString(
		"is_online",
		L.NewFunction(func(ls *lua.LState) int {
			ls.Push(
				lua.LBool(
					deps.Hub.IsOnline(
						ls.CheckString(1),
					),
				),
			)

			return 1
		}),
	)

	h.RawSetString(
		"kick",
		L.NewFunction(func(ls *lua.LState) int {
			ls.Push(
				lua.LNumber(
					deps.Hub.Kick(
						ls.CheckString(1),
					),
				),
			)

			return 1
		}),
	)

	h.RawSetString(
		"snapshot",
		L.NewFunction(func(ls *lua.LState) int {
			snap := deps.Hub.Snapshot()

			t := ls.NewTable()

			for uuid, n := range snap {
				t.RawSetString(
					uuid,
					lua.LNumber(n),
				)
			}

			ls.Push(t)

			return 1
		}),
	)

	L.Push(h)

	return 1
}

func goToLua(L *lua.LState, v any) lua.LValue {
	switch t := v.(type) {
	case nil:
		return lua.LNil

	case []byte:
		return lua.LString(string(t))

	case string:
		return lua.LString(t)

	case int64:
		return lua.LNumber(t)

	case float64:
		return lua.LNumber(t)

	case bool:
		return lua.LBool(t)

	default:
		return lua.LString(fmt.Sprint(t))
	}
}

func luaToArgs(L *lua.LState, n int) []any {
	top := L.GetTop()

	args := []any{}

	for i := n; i <= top; i++ {
		switch v := L.Get(i).(type) {
		case lua.LNumber:
			args = append(args, float64(v))

		case lua.LString:
			args = append(args, string(v))

		default:
			args = append(
				args,
				L.ToStringMeta(v).String(),
			)
		}
	}

	return args
}