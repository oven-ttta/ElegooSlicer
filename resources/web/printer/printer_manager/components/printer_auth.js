const PrinterAuthTemplate =
    /*html*/
    `
        <div class="printer-auth-dialog">
            <div class="printer-auth-content">
                <div class="printer-image-container">
                    <img :src="(printer && printer.printerImg) || ''" />
                </div>
                <h3>{{ (printer && printer.printerName) || '' }}</h3>
                <p class="instruction-text">Please enter the 6-character access code on the printer</p>
                
                <div class="access-code-inputs">
                    <el-input 
                        v-for="(code, index) in accessCodes" 
                        :key="index"
                        :ref="el => setInputRef(el, index)"
                        type="text" 
                        class="code-input" 
                        maxlength="1" 
                        v-model="accessCodes[index]"
                        @input="onCodeInput(index, $event)"
                        @keydown="onCodeKeydown(index, $event)"
                    />
                </div>
                
                <a href="#" class="help-link" @click.prevent="showHelp">Cannot access code?</a>

                
            </div>
            <div class="printer-auth-btn">
                <button class="btn-secondary" @click="closeModal">Close</button>
                <button class="btn-primary" @click="connectPrinter">Connect</button>
            </div>
            
            <!-- Help Dialog -->
            <el-dialog 
                v-model="showHelpDialog"
                title="Help"
                width="400px"
                center
            >
                <p>Please check the printer screen or manual for the 6-character access code.</p>
                <template #footer>
                    <span class="dialog-footer">
                        <button class="btn-primary" @click="showHelpDialog = false">OK</button>
                    </span>
                </template>
            </el-dialog>
        </div>
    `;

// Printer Auth Component
const PrinterAuthComponent = {
    template: PrinterAuthTemplate,

    props: {
        printer: {
            type: Object,
            default: null,
        },
    },

    emits: ["close-modal", "add-printer"],

    data() {
        return {
            accessCodes: ["", "", "", "", "", ""],
            inputRefs: [],
            showHelpDialog: false,
        };
    },

    mounted() {
        this.$nextTick(() => {
            this.focusFirstInput();
        });
    },

    beforeUnmount() {
  
    },

    watch: {
        printer: {
            handler(newVal) {
                if (newVal) {
                    this.renderPrinterAuth(newVal);
                }
            },
            immediate: true,
        },
    },

    methods: {
        // set input box reference
        setInputRef(el, index) {
            if (el) {
                this.inputRefs[index] = el;
            }
        },

        renderPrinterAuth(printer) {
            if (!printer) return;

            // clear access code
            this.accessCodes = ["", "", "", "", "", ""];

            // set focus to first input box
            this.$nextTick(() => {
                this.focusFirstInput();
            });
        },

        getHeaderTitle() {
            const authMode = this.printer && this.printer.authMode;

            switch (authMode) {
                case "token":
                    return "Connect to Printer";
                case "pin":
                    return "Bind Printer";
                default:
                    return "Connect to Printer";
            }
        },

        closeModal() {
            this.$emit("close-modal");
        },

        connectPrinter() {
            // merge all input values
            const accessCode = this.accessCodes.join("");

            if (accessCode.length !== 6) {
                alert("Please enter the complete 6-character access code");
                return;
            }

            const updatedPrinter = {
                ...this.printer,
                extraInfo: {
                    token: accessCode,
                },
            };

            this.$emit("add-printer", updatedPrinter);
        },

        onCodeInput(index, value) {
            // Element Plus @input event directly passes value, not event object
            console.log('onCodeInput called:', index, value);

            // ensure value is a string
            const stringValue = String(value || '');

            // only allow numbers and uppercase letters
            if (!/^[a-zA-Z0-9]*$/.test(stringValue)) {
                this.accessCodes[index] = "";
                return;
            }

            // if a character is entered, automatically focus on the next input box
            if (stringValue.length === 1 && index < 5) {
                this.$nextTick(() => {
                    const nextInput = this.inputRefs[index + 1];
                    if (nextInput) {
                        // Element Plus input focus setting
                        if (nextInput.focus) {
                            nextInput.focus();
                        } else if (nextInput.$refs && nextInput.$refs.input) {
                            nextInput.$refs.input.focus();
                        }
                    }
                });
            }
        },

        onCodeKeydown(index, event) {
            // handle backspace key
            if (
                event.key === "Backspace" &&
                this.accessCodes[index] === "" &&
                index > 0
            ) {
                this.$nextTick(() => {
                    const prevInput = this.inputRefs[index - 1];
                    if (prevInput) {
                        // Element Plus input box focus setting
                        if (prevInput.focus) {
                            prevInput.focus();
                        } else if (prevInput.$refs && prevInput.$refs.input) {
                            prevInput.$refs.input.focus();
                        }
                    }
                });
            }
        },

        focusFirstInput() {
            this.$nextTick(() => {
                const firstInput = this.inputRefs[0];
                if (firstInput) {
                    // Element Plus input box focus setting
                    if (firstInput.focus) {
                        firstInput.focus();
                    } else if (firstInput.$refs && firstInput.$refs.input) {
                        firstInput.$refs.input.focus();
                    }
                }
            });
        },

        showHelp() {
            this.showHelpDialog = true;
        },
    },
};
