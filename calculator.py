"""科学计算器 - Python tkinter 实现"""

import tkinter as tk
import math


class Calculator:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("科学计算器")
        self.root.geometry("420x620")
        self.root.resizable(False, False)
        self.root.configure(bg="#1e1e2e")

        self.expression = ""
        self.result_var = tk.StringVar(value="0")

        self._build_ui()

    def _build_ui(self):
        # 显示区域
        display_frame = tk.Frame(self.root, bg="#1e1e2e")
        display_frame.pack(fill="x", padx=16, pady=(16, 8))

        self.expr_label = tk.Label(
            display_frame, text="", font=("Segoe UI", 14),
            bg="#1e1e2e", fg="#a6adc8", anchor="e"
        )
        self.expr_label.pack(fill="x")

        self.result_label = tk.Label(
            display_frame, textvariable=self.result_var,
            font=("Segoe UI", 36, "bold"),
            bg="#1e1e2e", fg="#cdd6f4", anchor="e"
        )
        self.result_label.pack(fill="x")

        # 按钮区域
        btn_frame = tk.Frame(self.root, bg="#1e1e2e")
        btn_frame.pack(fill="both", expand=True, padx=12, pady=(0, 12))

        # 按钮布局: [文本, 列跨度, 颜色类型]
        buttons = [
            # 科学函数行
            [("sin", 1, "sci"), ("cos", 1, "sci"), ("tan", 1, "sci"),
             ("log", 1, "sci"), ("ln", 1, "sci"), ("√", 1, "sci")],
            # 更多科学函数
            [("x²", 1, "sci"), ("xʸ", 1, "sci"), ("π", 1, "sci"),
             ("e", 1, "sci"), ("n!", 1, "sci"), ("1/x", 1, "sci")],
            # 括号和清除
            [("C", 1, "func"), ("⌫", 1, "func"), ("(", 1, "func"),
             (")", 1, "func"), ("%", 1, "func"), ("÷", 1, "op")],
            # 数字行
            [("7", 1, "num"), ("8", 1, "num"), ("9", 1, "num"),
             ("×", 1, "op"), ("±", 1, "func"), ("", 0, "")],
            [("4", 1, "num"), ("5", 1, "num"), ("6", 1, "num"),
             ("−", 1, "op"), ("", 0, ""), ("", 0, "")],
            [("1", 1, "num"), ("2", 1, "num"), ("3", 1, "num"),
             ("+", 1, "op"), ("", 0, ""), ("", 0, "")],
            [("0", 2, "num"), (".", 1, "num"), ("=", 1, "eq"),
             ("", 0, ""), ("", 0, ""), ("", 0, "")],
        ]

        colors = {
            "num":  {"bg": "#313244", "fg": "#cdd6f4", "hover": "#45475a"},
            "op":   {"bg": "#45475a", "fg": "#89b4fa", "hover": "#585b70"},
            "func": {"bg": "#45475a", "fg": "#f9e2af", "hover": "#585b70"},
            "sci":  {"bg": "#313244", "fg": "#a6e3a1", "hover": "#45475a"},
            "eq":   {"bg": "#89b4fa", "fg": "#1e1e2e", "hover": "#74c7ec"},
        }

        for r, row in enumerate(buttons):
            btn_frame.rowconfigure(r, weight=1)
            col = 0
            for text, colspan, style in row:
                if not text:
                    col += colspan
                    continue
                btn_frame.columnconfigure(col, weight=1)
                c = colors.get(style, colors["num"])
                btn = tk.Button(
                    btn_frame, text=text,
                    font=("Segoe UI", 16),
                    bg=c["bg"], fg=c["fg"],
                    activebackground=c["hover"], activeforeground=c["fg"],
                    relief="flat", bd=0,
                    command=lambda t=text: self._on_click(t)
                )
                btn.grid(
                    row=r, column=col, columnspan=colspan,
                    sticky="nsew", padx=3, pady=3
                )
                btn.bind("<Enter>", lambda e, b=btn, h=c["hover"]: b.configure(bg=h))
                btn.bind("<Leave>", lambda e, b=btn, n=c["bg"]: b.configure(bg=n))
                col += colspan

    def _on_click(self, key):
        if key == "C":
            self.expression = ""
            self.result_var.set("0")
            self.expr_label.config(text="")
            return

        if key == "⌫":
            self.expression = self.expression[:-1]
            self.expr_label.config(text=self.expression)
            if not self.expression:
                self.result_var.set("0")
            return

        if key == "=":
            self._evaluate()
            return

        if key == "±":
            if self.expression:
                if self.expression.startswith("-"):
                    self.expression = self.expression[1:]
                else:
                    self.expression = "-" + self.expression
                self.expr_label.config(text=self.expression)
            return

        # 科学函数 - 直接计算
        sci_funcs = {
            "sin": "math.sin(", "cos": "math.cos(", "tan": "math.tan(",
            "log": "math.log10(", "ln": "math.log(", "√": "math.sqrt(",
            "n!": "math.factorial(", "1/x": "reciprocal",
        }

        if key in sci_funcs:
            if key == "1/x":
                self.expression = f"1/({self.expression})" if self.expression else "1/"
            else:
                self.expression += sci_funcs[key]
            self.expr_label.config(text=self.expression)
            return

        # 常量
        if key == "π":
            self.expression += "math.pi"
            self.expr_label.config(text=self.expression)
            return
        if key == "e":
            self.expression += "math.e"
            self.expr_label.config(text=self.expression)
            return

        # 幂运算
        if key == "x²":
            self.expression += "**2"
            self.expr_label.config(text=self.expression)
            return
        if key == "xʸ":
            self.expression += "**"
            self.expr_label.config(text=self.expression)
            return

        # 运算符显示转换
        display_map = {"×": "*", "−": "-", "÷": "/"}
        expr_key = display_map.get(key, key)

        self.expression += expr_key
        self.expr_label.config(text=self.expression)

        # 尝试实时预览结果
        try:
            result = eval(self.expression, {"math": math, "__builtins__": {}})
            if isinstance(result, float) and result == int(result) and abs(result) < 1e15:
                result = int(result)
            self.result_var.set(str(result))
        except Exception:
            pass

    def _evaluate(self):
        if not self.expression:
            return
        try:
            expr = self.expression
            result = eval(expr, {"math": math, "__builtins__": {}})
            if isinstance(result, float):
                if result == int(result) and abs(result) < 1e15:
                    result = int(result)
                else:
                    result = round(result, 12)
            self.result_var.set(str(result))
            self.expression = str(result)
        except ZeroDivisionError:
            self.result_var.set("错误: 除以零")
            self.expression = ""
        except Exception:
            self.result_var.set("错误")
            self.expression = ""

    def run(self):
        self.root.mainloop()


if __name__ == "__main__":
    Calculator().run()
