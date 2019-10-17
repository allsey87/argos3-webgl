class Editor {
    constructor(codeMirror) {
        this.scripts = [];
        this.codeMirror = codeMirror;
    }

    selectionChanged(selection) {
        if (selection && selection.luaScript) {
            const newContent = this.scripts[selection.luaScript];
            this.codeMirror.getDoc().setValue(newContent);
        } else {
            this.codeMirror.getDoc().setValue("-- No Lua script for this robot");
        }
    }

    setScripts = (scripts) => {
        scripts.forEach((script) => {
            this.scripts[script.name] = script.content;
        });
    }
}