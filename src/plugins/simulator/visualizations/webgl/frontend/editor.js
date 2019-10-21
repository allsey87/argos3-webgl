class Editor {
    constructor(codeMirror, saveButton) {
        this.scripts = [];
        this.codeMirror = codeMirror;
        this.currentScript = undefined;
        saveButton.onclick = this.clicked.bind(this);
    }

    clicked() {
        if (this.currentScript) {
            websocket.send({
                "messageType": "luaScript",
                "file": this.currentScript,
                "content": this.codeMirror.getDoc().getValue()
            });
        }
    }

    selectionChanged(selection) {
        if (selection && selection.luaScript) {
            const newContent = this.scripts[selection.luaScript];
            this.codeMirror.getDoc().setValue(newContent);
            this.currentScript = selection.luaScript;
        } else {
            this.currentScript = undefined;
            this.codeMirror.getDoc().setValue("-- No Lua script for this robot");
        }
    }

    setScripts = (scripts) => {
        scripts.forEach((script) => {
            this.scripts[script.name] = script.content;
        });
    }
}