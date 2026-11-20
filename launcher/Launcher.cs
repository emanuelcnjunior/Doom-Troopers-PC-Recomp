using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Windows.Forms;

internal sealed class Launcher : Form
{
    private static readonly string Folder = AppDomain.CurrentDomain.BaseDirectory;
    private static string SettingsFile(string section)
    {
        return Path.Combine(Folder, section == "doom_launcher" ? "launcher.ini" : "settings.ini");
    }
    private static readonly string[] Actions = { "Atirar", "Pular", "Atacar com a arma" };
    private static readonly string[] Keys = { "pad.B", "pad.C", "pad.A" };
    private static readonly string[] Buttons = { "X (cruz)", "Bolinha", "Quadrado", "Triângulo" };
    private static readonly int[] Defaults = { 2, 0, 1 };
    private readonly ComboBox[] bindings = new ComboBox[3];
    private readonly TextBox rom = new TextBox();
    private readonly CheckBox fullscreen = new CheckBox();
    private readonly CheckBox[] cheats = new CheckBox[30];
    private const string RomHash = "d2fedd7f024571a4a30d241bca2d66fcd5db60283914c792c611f688b83ad3a2";

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    private static extern uint GetPrivateProfileString(string section, string key, string fallback,
        StringBuilder result, uint size, string path);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool WritePrivateProfileString(string section, string key, string value, string path);

    private static string Read(string section, string key, string fallback)
    {
        var value = new StringBuilder(4096);
        GetPrivateProfileString(section, key, fallback, value, (uint)value.Capacity, SettingsFile(section));
        return value.ToString().Split(';')[0].Trim();
    }

    private void Save(string section, string key, string value)
    {
        if (!WritePrivateProfileString(section, key, value, SettingsFile(section)))
            throw new IOException(Translate("O Windows não conseguiu salvar as configurações na pasta do jogo."));
    }

    private static readonly Color Night = Color.FromArgb(16, 22, 29);
    private static readonly Color Surface = Color.FromArgb(27, 37, 46);
    private static readonly Color Ink = Color.FromArgb(235, 239, 234);
    private static readonly Color Muted = Color.FromArgb(167, 184, 190);
    private static readonly Color Amber = Color.FromArgb(255, 178, 76);

    private static readonly string[] CheatNames = {
        "Energia infinita · P1", "Vidas infinitas · P1", "Munição infinita · P1",
        "Energia infinita · P2", "Vidas infinitas · P2", "Munição infinita · P2",
        "Pular fases", "Velocidade dupla", "Vidas infinitas", "Energia infinita",
        "Munição infinita", "Vidas infinitas no modo de 2 jogadores",
        "Invencibilidade após um golpe", "Habilitar invencibilidade para ambos",
        "Selecionar fase após escolher o personagem", "Continuações infinitas",
        "Reduzir dano para ambos", "Começar com 200 munições", "Começar com 300 munições",
        "Começar com 500 munições", "Começar com 999 munições", "Começar com 5 vidas",
        "Começar com 9 vidas", "Começar com 5 continuações", "Começar com 9 continuações",
        "Munição infinita para ambos", "Coletar 100 munições por item",
        "Dobrar duração da invencibilidade", "Triplicar duração da invencibilidade",
        "Multiplicar duração da invencibilidade por 5"
    };
    private bool english;
    private readonly Dictionary<Control, string> captions = new Dictionary<Control, string>();
    private static readonly Dictionary<string, string> English = new Dictionary<string, string> {
        { "Controles", "Controls" },
        { "Jogador 1", "Player 1" },
        { "Atirar", "Shoot" },
        { "Pular", "Jump" },
        { "Atacar com a arma", "Melee attack" },
        { "L2   Salvar partida", "L2   Save state" },
        { "R2   Carregar partida", "R2   Load state" },
        { "Restaurar padrão", "Restore defaults" },
        { "Ative os códigos para a próxima partida.", "Choose cheats for your next game." },
        { "ROM do Mega Drive", "Genesis / Mega Drive ROM" },
        { "Procurar…", "Browse…" },
        { "Tela cheia", "Fullscreen" },
        { "Jogar", "Play" },
        { "Idioma", "Language" },
        { "ROM do Mega Drive|*.md;*.bin;*.gen|Todos os arquivos|*.*", "Genesis / Mega Drive ROM|*.md;*.bin;*.gen|All files|*.*" },
        { "O Windows não conseguiu salvar as configurações na pasta do jogo.", "Windows could not save your settings in the game folder." },
        { "Selecione sua ROM de Doom Troopers (USA).", "Select your Doom Troopers (USA) ROM." },
        { "A ROM selecionada não corresponde à revisão USA do pacote.", "Select the supported USA revision of Doom Troopers." },
        { "Escolha um botão diferente para cada ação.", "Choose a different button for each action." },
        { "DoomBootProbe.exe está faltando na pasta do launcher.", "DoomBootProbe.exe is missing from the launcher folder." },
        { "Energia infinita · P1", "Infinite health · P1" },
        { "Vidas infinitas · P1", "Infinite lives · P1" },
        { "Munição infinita · P1", "Infinite ammo · P1" },
        { "Energia infinita · P2", "Infinite health · P2" },
        { "Vidas infinitas · P2", "Infinite lives · P2" },
        { "Munição infinita · P2", "Infinite ammo · P2" },
        { "Pular fases", "Skip levels" },
        { "Velocidade dupla", "Double speed" },
        { "Vidas infinitas", "Infinite lives" },
        { "Energia infinita", "Infinite health" },
        { "Munição infinita", "Infinite ammo" },
        { "Vidas infinitas no modo de 2 jogadores", "Infinite lives in two-player mode" },
        { "Invencibilidade após um golpe", "Invincibility after one hit" },
        { "Habilitar invencibilidade para ambos", "Enable invincibility for both players" },
        { "Selecionar fase após escolher o personagem", "Level select after character selection" },
        { "Continuações infinitas", "Infinite continues" },
        { "Reduzir dano para ambos", "Both players take less damage" },
        { "Começar com 200 munições", "Start with 200 rounds" },
        { "Começar com 300 munições", "Start with 300 rounds" },
        { "Começar com 500 munições", "Start with 500 rounds" },
        { "Começar com 999 munições", "Start with 999 rounds" },
        { "Começar com 5 vidas", "Start with 5 lives" },
        { "Começar com 9 vidas", "Start with 9 lives" },
        { "Começar com 5 continuações", "Start with 5 continues" },
        { "Começar com 9 continuações", "Start with 9 continues" },
        { "Munição infinita para ambos", "Infinite ammo for both players" },
        { "Coletar 100 munições por item", "Collect 100 rounds per ammo pickup" },
        { "Dobrar duração da invencibilidade", "Double invincibility time after a hit" },
        { "Triplicar duração da invencibilidade", "Triple invincibility time after a hit" },
        { "Multiplicar duração da invencibilidade por 5", "Five times the invincibility time after a hit" },
    };

    private string Translate(string original)
    {
        string translated;
        return english && English.TryGetValue(original, out translated) ? translated : original;
    }

    private void CollectCaptions(Control parent)
    {
        foreach (Control child in parent.Controls)
        {
            if (English.ContainsKey(child.Text) && (child is Label || child is ButtonBase))
                captions.Add(child, child.Text);
            CollectCaptions(child);
        }
    }

    private void ApplyLanguage(bool useEnglish)
    {
        english = useEnglish;
        foreach (var caption in captions)
            caption.Key.Text = Translate(caption.Value);
        string[] names = english ? new[] { "X (cross)", "Circle", "Square", "Triangle" } : Buttons;
        foreach (ComboBox box in bindings)
        {
            int selected = box.SelectedIndex;
            box.Items.Clear();
            box.Items.AddRange(names);
            box.SelectedIndex = selected;
        }
    }

    private static readonly int[][] Exclusive = {
        new[] { 6, 7 }, new[] { 17, 18, 19, 20 }, new[] { 21, 22 },
        new[] { 23, 24 }, new[] { 27, 28, 29 }
    };

    private static Label TextAt(Control host, string text, int x, int y, int w, int h,
                                float size, Color color, bool bold = false)
    {
        var label = new Label { Text = text, ForeColor = color, BackColor = Color.Transparent,
            Bounds = new Rectangle(x, y, w, h),
            Font = new Font("Segoe UI", size, bold ? FontStyle.Bold : FontStyle.Regular) };
        host.Controls.Add(label);
        return label;
    }

    private static Button ButtonAt(Control host, string text, int x, int y, int w, int h)
    {
        var button = new Button { Text = text, ForeColor = Ink, BackColor = Surface,
            FlatStyle = FlatStyle.Flat, Cursor = Cursors.Hand, Bounds = new Rectangle(x,y,w,h) };
        button.FlatAppearance.BorderSize = 0;
        host.Controls.Add(button);
        return button;
    }

    private Launcher(string[] args)
    {
        Text = "Doom Troopers";
        Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath);
        StartPosition = FormStartPosition.CenterScreen;
        Font = new Font("Segoe UI", 10);
        AutoScaleDimensions = new SizeF(96, 96);
        AutoScaleMode = AutoScaleMode.Dpi;
        ClientSize = new Size(800, 540);
        BackColor = Night;
        ForeColor = Ink;
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;

        var rail = new Panel { Bounds = new Rectangle(0,0,210,540), BackColor = Surface };
        Controls.Add(rail);
        var sleeve = new Panel { Bounds = new Rectangle(10,14,190,270),
            Padding = new Padding(2), BackColor = Color.FromArgb(164,118,62) };
        rail.Controls.Add(sleeve);
        using (var stream = typeof(Launcher).Assembly.GetManifestResourceStream("cover.jpg"))
        using (var cover = new Bitmap(stream))
            sleeve.Controls.Add(new PictureBox { Image = new Bitmap(cover),
                BackColor = Night, SizeMode = PictureBoxSizeMode.Zoom, Dock = DockStyle.Fill });
        var controlsNav = ButtonAt(rail,"Controles",16,318,178,43);
        var cheatsNav = ButtonAt(rail,"Cheats",16,367,178,43);
        TextAt(rail,"Idioma",22,423,170,23,10,Muted,true);
        var language = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList,
            FlatStyle = FlatStyle.Flat, Bounds = new Rectangle(22,450,164,28) };
        language.Items.AddRange(new[] { "English", "Português" });
        rail.Controls.Add(language);
        TextAt(rail,"MEGA DRIVE  /  USA",22,494,175,26,9,Muted,true);

        var pad = new Panel { Bounds = new Rectangle(232,18,546,330) };
        var options = new Panel { Bounds = pad.Bounds };
        Controls.Add(pad); Controls.Add(options);
        Action<bool> show = delegate(bool showCheats) {
            pad.Visible = !showCheats; options.Visible = showCheats;
            controlsNav.ForeColor = showCheats ? Muted : Night;
            cheatsNav.ForeColor = showCheats ? Night : Muted;
            controlsNav.BackColor = showCheats ? Surface : Amber;
            cheatsNav.BackColor = showCheats ? Amber : Surface;
        };
        controlsNav.Click += delegate { show(false); };
        cheatsNav.Click += delegate { show(true); };
        show(false);
        var marquee = new Panel { Bounds = new Rectangle(0,0,536,50), AccessibleName = "Doom Troopers" };
        marquee.Paint += delegate(object sender, PaintEventArgs e) {
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            using (var letters = new GraphicsPath())
            using (var face = new FontFamily("Georgia"))
            using (var outline = new Pen(Color.FromArgb(125,48,30), 3))
            using (var gold = new LinearGradientBrush(new Rectangle(0,4,1,39),
                    Color.FromArgb(255,236,165), Color.FromArgb(230,148,46), 90f))
            {
                letters.AddString("DOOM TROOPERS", face, (int)FontStyle.Bold, 36,
                    new PointF(0,0), StringFormat.GenericTypographic);
                g.DrawPath(outline, letters);
                g.FillPath(gold, letters);
            }
            using (var stripe = new SolidBrush(Color.FromArgb(166,52,44)))
                g.FillRectangle(stripe, 2, 46, marquee.ClientSize.Width - 4, 3);
        };
        pad.Controls.Add(marquee);
        TextAt(pad,"Jogador 1",2,55,300,25,11,Muted);
        for (int i = 0; i < bindings.Length; i++)
        {
            int y = 92 + 42 * i;
            TextAt(pad,Actions[i],4,y+2,235,30,11,Ink);
            var box = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList,
                FlatStyle = FlatStyle.Flat, BackColor = Surface, ForeColor = Ink,
                Bounds = new Rectangle(278,y,258,30) };
            box.Items.AddRange(Buttons);
            int saved;
            string value = Read("input.p1", Keys[i], "");
            box.SelectedIndex = value.StartsWith("button:") && int.TryParse(value.Substring(7), out saved)
                && saved >= 0 && saved < Buttons.Length ? saved : Defaults[i];
            bindings[i] = box; pad.Controls.Add(box);
        }
        TextAt(pad,"L2   Salvar partida",4,234,254,28,11,Amber,true);
        TextAt(pad,"R2   Carregar partida",278,234,257,28,11,Amber,true);
        var reset = ButtonAt(pad,"Restaurar padrão",278,280,258,35);
        reset.Click += delegate {
            for (int i = 0; i < bindings.Length; i++) bindings[i].SelectedIndex = Defaults[i];
        };
        TextAt(options,"Cheats",0,2,320,45,23,Ink,true);
        TextAt(options,"Ative os códigos para a próxima partida.",2,54,520,26,11,Muted);
        var list = new Panel { Bounds = new Rectangle(0,89,542,236), AutoScroll = true, BackColor = Surface };
        options.Controls.Add(list);
        int mask;
        int.TryParse(Read("doom_launcher","cheats","0"),out mask);
        for (int i = 0; i < CheatNames.Length; ++i)
        {
            int index = i;
            var check = new CheckBox { Text = CheatNames[i], ForeColor = Ink, Bounds = new Rectangle(14,8+32*i,495,30) };
            cheats[i] = check; list.Controls.Add(check);
            check.CheckedChanged += delegate {
                if (!check.Checked) return;
                foreach (int[] group in Exclusive)
                    if (Array.IndexOf(group,index) >= 0)
                        foreach (int other in group)
                            if (other != index && cheats[other] != null) cheats[other].Checked = false;
            };
            check.Checked = (mask & (1 << i)) != 0;
        }
        Controls.Add(new Panel { BackColor = Surface, Bounds = new Rectangle(232,357,546,2) });
        TextAt(this,"ROM do Mega Drive",234,373,440,25,10,Muted,true);
        rom.SetBounds(234,407,420,30); rom.BackColor=Surface; rom.ForeColor=Ink;
        rom.BorderStyle = BorderStyle.FixedSingle;
        string remembered = Read("doom_launcher","rom_utf8","");
        try { remembered = Encoding.UTF8.GetString(Convert.FromBase64String(remembered)); }
        catch (FormatException) { remembered = ""; }
        string adjacent = Path.Combine(Folder, "Doom Troopers (USA).md");
        rom.Text = args.Length > 0 ? args[0] : File.Exists(remembered) ? remembered :
            File.Exists(adjacent) ? adjacent : "";
        Controls.Add(rom);
        var browse = ButtonAt(this,"Procurar…",664,402,113,34);
        browse.Click += delegate {
            using (var dlg = new OpenFileDialog { Filter = Translate("ROM do Mega Drive|*.md;*.bin;*.gen|Todos os arquivos|*.*") })
                if (dlg.ShowDialog(this) == DialogResult.OK) rom.Text = dlg.FileName;
        };
        fullscreen.Text = "Tela cheia";
        fullscreen.Checked = Read("video","fullscreen","1") != "0";
        fullscreen.SetBounds(234,468,150,40); Controls.Add(fullscreen);
        var play = ButtonAt(this,"Jogar",578,462,199,48); play.BackColor = Amber; play.ForeColor = Night;
        play.Font = new Font(Font, FontStyle.Bold);
        play.Click += delegate { Launch(); }; AcceptButton = play;
        CollectCaptions(this);
        language.SelectedIndex = Read("doom_launcher", "language", "en") == "pt" ? 1 : 0;
        ApplyLanguage(language.SelectedIndex == 0);
        language.SelectedIndexChanged += delegate {
            ApplyLanguage(language.SelectedIndex == 0);
            try { Save("doom_launcher", "language", english ? "en" : "pt"); }
            catch (IOException ex) { MessageBox.Show(this, ex.Message, Text); }
        };
    }

    private void Launch()
    {
        try
        {
            string path = Path.GetFullPath(rom.Text.Trim().Trim('"'));
            if (!File.Exists(path)) throw new IOException(Translate("Selecione sua ROM de Doom Troopers (USA)."));
            using (var stream = File.OpenRead(path))
            using (var sha = SHA256.Create())
                if (stream.Length != 2097152 || BitConverter.ToString(sha.ComputeHash(stream)).Replace("-", "")
                    .ToLowerInvariant() != RomHash)
                    throw new InvalidDataException(Translate("A ROM selecionada não corresponde à revisão USA do pacote."));
            int used = 0;
            foreach (var box in bindings)
            {
                int bit = 1 << box.SelectedIndex;
                if ((used & bit) != 0) throw new InvalidDataException(Translate("Escolha um botão diferente para cada ação."));
                used |= bit;
            }
            int mask = 0;
            for (int i = 0; i < cheats.Length; i++) if (cheats[i].Checked) mask |= 1 << i;
            var game = Path.Combine(Folder, "DoomBootProbe.exe");
            if (!File.Exists(game)) throw new FileNotFoundException(Translate("DoomBootProbe.exe está faltando na pasta do launcher."));
            for (int i = 0; i < bindings.Length; ++i)
                Save("input.p1", Keys[i], "button:" + bindings[i].SelectedIndex);
            Save("video", "fullscreen", fullscreen.Checked ? "1" : "0");
            Save("doom_launcher", "rom_utf8", Convert.ToBase64String(Encoding.UTF8.GetBytes(path)));
            Save("doom_launcher", "cheats", mask.ToString());
            var info = new ProcessStartInfo(game, "\"" + path + "\"") {
                WorkingDirectory = Folder, UseShellExecute = false, CreateNoWindow = true };
            info.EnvironmentVariables["GENESIS_STRICT_JSR_STACK"] = "1";
            info.EnvironmentVariables["DOOM_CHEAT_MASK"] = mask.ToString();
            if ((mask & ~0x103f) != 0) info.EnvironmentVariables["GENESIS_FORCE_INTERP"] = "1";
            using (Process child = Process.Start(info)) { }
            Close();
        }
        catch (Exception ex) { MessageBox.Show(this, ex.Message, Text, MessageBoxButtons.OK, MessageBoxIcon.Warning); }
    }

    [STAThread]
    private static void Main(string[] args)
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new Launcher(args));
    }
}
