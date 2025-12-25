package com.example.esdl_term_project

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.Context
import android.os.Build
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.*
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.esdl_term_project.ui.theme.Esdl_term_projectTheme
import com.example.esdl_term_project.ui.theme.NeonAmber
import com.example.esdl_term_project.ui.theme.NeonCyan
import com.example.esdl_term_project.ui.theme.NeonMagenta
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {

    private lateinit var bluetoothManagerWrapper: com.example.esdl_term_project.BluetoothManager

    private val requestPermissionLauncher =
        registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { permissions ->
            val granted = permissions.entries.all { it.value }
            if (!granted) {
                Toast.makeText(this, "Bluetooth permissions are required", Toast.LENGTH_LONG).show()
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        val adapter = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            val bm = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
            bm.adapter
        } else {
            BluetoothAdapter.getDefaultAdapter()
        }
        
        bluetoothManagerWrapper = com.example.esdl_term_project.BluetoothManager(adapter)

        setContent {
            Esdl_term_projectTheme {
                // Cyberpunk / Retro Grid Background
                val infiniteTransition = rememberInfiniteTransition(label = "grid_anim")
                val offset by infiniteTransition.animateFloat(
                    initialValue = 0f,
                    targetValue = 100f,
                    animationSpec = infiniteRepeatable(
                        animation = tween(2000, easing = LinearEasing),
                        repeatMode = RepeatMode.Restart
                    ),
                    label = "grid_offset"
                )

                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .background(Color(0xFF050510)) // Deep dark blue/black
                ) {
                    // Moving Grid Effect (Simulated)
                    Canvas(modifier = Modifier.fillMaxSize().alpha(0.2f)) {
                        val gridSize = 50.dp.toPx()
                        val height = size.height
                        val width = size.width
                        
                        // Vertical lines
                        for (i in 0..((width/gridSize).toInt())) {
                            drawLine(
                                color = NeonCyan,
                                start = androidx.compose.ui.geometry.Offset(i * gridSize, 0f),
                                end = androidx.compose.ui.geometry.Offset(i * gridSize, height),
                                strokeWidth = 1f
                            )
                        }
                        
                        // Horizontal lines (moving)
                        val startY = offset % gridSize
                        for (i in 0..((height/gridSize).toInt() + 1)) {
                            val y = startY + (i * gridSize) - gridSize
                            drawLine(
                                color = NeonMagenta,
                                start = androidx.compose.ui.geometry.Offset(0f, y),
                                end = androidx.compose.ui.geometry.Offset(width, y),
                                strokeWidth = 1f
                            )
                        }
                    }
                    
                    // Vignette
                    Box(
                         modifier = Modifier
                             .fillMaxSize()
                             .background(
                                 Brush.radialGradient(
                                     colors = listOf(Color.Transparent, Color.Black.copy(alpha = 0.8f)),
                                     radius = 1200f
                                 )
                             )
                    )

                    Scaffold(
                        modifier = Modifier.fillMaxSize(),
                        containerColor = Color.Transparent
                    ) { innerPadding ->
                        Box(modifier = Modifier.padding(innerPadding)) {
                            val connectionState by bluetoothManagerWrapper.connectionState.collectAsState()
                            val gameStatus by bluetoothManagerWrapper.gameStatus.collectAsState()
                            val messageLogs by bluetoothManagerWrapper.messageLog.collectAsState()
                            val scope = rememberCoroutineScope()
    
                            GameScreen(
                                connectionState = connectionState,
                                gameStatus = gameStatus,
                                onConnect = { device ->
                                    scope.launch { bluetoothManagerWrapper.connect(device) }
                                },
                                onDisconnect = { bluetoothManagerWrapper.disconnect() },
                                onSendMessage = { message ->
                                     scope.launch { bluetoothManagerWrapper.sendMessage(message) }
                                },
                                onCheckPermissions = { checkPermissions() },
                                bluetoothAdapter = adapter,
                                messageLogs = messageLogs
                            )
                        }
                    }
                }
            }
        }
    }
    
    private fun checkPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            requestPermissionLauncher.launch(
                arrayOf(
                    Manifest.permission.BLUETOOTH_SCAN,
                    Manifest.permission.BLUETOOTH_CONNECT
                )
            )
        } else {
             requestPermissionLauncher.launch(
                arrayOf(
                    Manifest.permission.ACCESS_FINE_LOCATION,
                    Manifest.permission.ACCESS_COARSE_LOCATION
                )
            )
        }
    }
}

@Composable
fun GameScreen(
    connectionState: ConnectionState,
    gameStatus: GameStatus?,
    onConnect: (android.bluetooth.BluetoothDevice) -> Unit,
    onDisconnect: () -> Unit,
    onSendMessage: (String) -> Unit,
    modifier: Modifier = Modifier,
    onCheckPermissions: () -> Unit,
    bluetoothAdapter: BluetoothAdapter?,
    messageLogs: List<String>
) {
    var showDeviceList by remember { mutableStateOf(false) }
    var showLogs by remember { mutableStateOf(false) }
    
    // Status Animations
    val isConnected = connectionState is ConnectionState.Connected
    val statusColor by animateColorAsState(
        targetValue = if (isConnected) NeonCyan else Color.Gray,
        label = "statusColor"
    )

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        // --- Header ---
        Spacer(modifier = Modifier.height(32.dp))
        Text(
            text = "STM32 DUCK HUNT",
            style = MaterialTheme.typography.displayMedium.copy(
                shadow = androidx.compose.ui.graphics.Shadow(
                    color = NeonMagenta,
                    blurRadius = 30f,
                    offset = androidx.compose.ui.geometry.Offset(0f, 0f)
                )
            ),
            color = Color.White,
            textAlign = TextAlign.Center
        )
        Text(
            text = "PNU CSE 2025 ESDL TEAM PROJECT",
            style = MaterialTheme.typography.labelMedium,
            color = NeonCyan.copy(alpha = 0.7f),
            letterSpacing = 6.sp,
            textAlign = TextAlign.Center
        )

        Spacer(modifier = Modifier.height(24.dp))

        // --- connection Status ---
        CyberpunkPanel(
            modifier = Modifier
                .fillMaxWidth()
                .clickable {
                    if (!isConnected) {
                        onCheckPermissions()
                        showDeviceList = true
                    } else {
                        onDisconnect()
                    }
                },
            borderColor = statusColor,
            backgroundColor = statusColor.copy(alpha = 0.05f)
        ) {
             Row(
                modifier = Modifier.padding(12.dp),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.Center
             ) {
                 Canvas(modifier = Modifier.size(10.dp)) {
                     drawCircle(color = statusColor)
                     drawCircle(color = statusColor.copy(alpha=0.5f), radius = 15f)
                 }
                 Spacer(modifier = Modifier.width(10.dp))
                 Text(
                    text = when (val state = connectionState) {
                        is ConnectionState.Connected -> "SYSTEM ONLINE: ${state.deviceName.uppercase()}"
                        is ConnectionState.Connecting -> "INITIATING LINK..."
                        is ConnectionState.Disconnected -> "SYSTEM OFFLINE - TAP TO CONNECT"
                        is ConnectionState.Error -> "SYSTEM ERROR: ${state.message.uppercase()}"
                    },
                    style = MaterialTheme.typography.labelLarge,
                    color = statusColor
                 )
             }
        }

        Spacer(modifier = Modifier.height(24.dp))

        // --- Main Game Monitor ---
        // --- Main Game Monitor ---
        Box(modifier = Modifier.weight(1f), contentAlignment = Alignment.Center) {
             if (isConnected) {
                 GameDashboard(gameStatus, onSendMessage)
             } else {
                 Text(
                     text = "LINK REQUIRED", 
                     color = Color.DarkGray, 
                     style = MaterialTheme.typography.displayMedium,
                     fontWeight = FontWeight.Bold 
                 )
             }
        }

        // --- Footer / Logs ---
        CyberpunkPanel(
            modifier = Modifier.fillMaxWidth(),
            borderColor = NeonCyan.copy(alpha = 0.3f),
            backgroundColor = Color.Black.copy(alpha = 0.6f)
        ) {
            Column(modifier = Modifier.padding(12.dp)) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text("DATA TERMINAL", color = NeonCyan.copy(alpha = 0.5f), fontSize = 10.sp)
                    TextButton(onClick = { showLogs = true }, modifier = Modifier.height(24.dp)) {
                         Text("EXPAND VIEW", color = NeonAmber, fontSize = 10.sp)
                    }
                }
                
                Spacer(modifier = Modifier.height(4.dp))
                
                Text(
                    text = if (messageLogs.isNotEmpty()) "> ${messageLogs.first()}" else "> NO DATA RECEIVED",
                    style = MaterialTheme.typography.bodySmall,
                    color = if(messageLogs.isNotEmpty()) NeonCyan else Color.Gray,
                    fontFamily = androidx.compose.ui.text.font.FontFamily.Monospace,
                    maxLines = 1
                )
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))

        if (showLogs) {
             LogViewerDialog(logs = messageLogs, onDismiss = { showLogs = false })
        }

        if (showDeviceList) {
            DeviceListDialog(
                adapter = bluetoothAdapter,
                onDismiss = { showDeviceList = false },
                onDeviceSelected = { device ->
                    showDeviceList = false
                    onConnect(device)
                }
            )
        }
    }
}

@Composable
fun GameDashboard(status: GameStatus?, onSendMessage: (String) -> Unit) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center, // Center everything vertically
        modifier = Modifier.fillMaxSize().padding(16.dp)
    ) {
        
        // --- SCOREBOARD ---
        Box(
            contentAlignment = Alignment.Center,
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 20.dp)
                .border(3.dp, NeonMagenta, RoundedCornerShape(24.dp))
                .background(NeonMagenta.copy(alpha = 0.05f), RoundedCornerShape(24.dp))
                .padding(vertical = 80.dp, horizontal = 16.dp)
                // Add outer shadow/glow via graphicsLayer if needed, but simple border is requested "neon rectangular border"
        ) {
             // Optional: Inner glow or fancier border effect could be added here
             
             Column(horizontalAlignment = Alignment.CenterHorizontally) {
                 Text(
                     text = "CURRENT SCORE",
                     style = MaterialTheme.typography.labelMedium,
                     color = NeonMagenta,
                     letterSpacing = 4.sp,
                     fontSize = 18.sp
                 )
                 Spacer(modifier = Modifier.height(8.dp))
                 Text(
                     text = String.format("%05d", status?.score ?: 0),
                     style = MaterialTheme.typography.displayLarge.copy(
                         fontSize = 96.sp, // Even larger
                         shadow = androidx.compose.ui.graphics.Shadow(
                             color = NeonMagenta,
                             blurRadius = 30f
                         )
                     ),
                     color = Color.White
                 )
             }
        }
        
        Spacer(modifier = Modifier.height(48.dp))

        // --- AMMO DISPLAY ---
        val maxAmmo = 3
        val currentAmmo = status?.bullets ?: 3
        
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text(
                "AMMUNITION", 
                color = NeonAmber, 
                style = MaterialTheme.typography.titleMedium,
                letterSpacing = 4.sp
            )
            Spacer(modifier = Modifier.height(24.dp))
             
            Row(
                horizontalArrangement = Arrangement.spacedBy(24.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                for (i in 1..maxAmmo) {
                    val isActive = i <= currentAmmo
                    
                    // Rifle Cartridge Shape (Refined: Less "Wing-like")
                    val bulletShape = androidx.compose.foundation.shape.GenericShape { size, _ ->
                        val w = size.width
                        val h = size.height
                        
                        // Start at Top Tip
                        moveTo(w / 2f, 0f)
                        
                        // Right Side
                        // Ogive (Bullet curve - wider neck)
                        cubicTo(w * 0.6f, 0f, w * 0.85f, h * 0.15f, w * 0.85f, h * 0.3f) 
                        
                        // Neck (Short vertical)
                        lineTo(w * 0.85f, h * 0.35f)
                        
                        // Shoulder (Subtler taper)
                        lineTo(w, h * 0.45f)
                        
                        // Body
                        lineTo(w, h * 0.9f)
                        
                        // Extractor Groove
                        lineTo(w * 0.9f, h * 0.9f)
                        lineTo(w * 0.9f, h * 0.94f)
                        
                        // Rim Flange
                        lineTo(w, h * 0.94f)
                        lineTo(w, h)
                        
                        // Top of Bottom (Base Line)
                        lineTo(0f, h)
                        
                        // Left Side (Mirror)
                        // Rim
                        lineTo(0f, h * 0.94f)
                        lineTo(w * 0.1f, h * 0.94f)
                        
                        // Groove
                        lineTo(w * 0.1f, h * 0.9f)
                        lineTo(0f, h * 0.9f)
                        
                        // Body
                        lineTo(0f, h * 0.45f)
                        
                        // Shoulder
                        lineTo(w * 0.15f, h * 0.35f)
                        
                        // Neck
                        lineTo(w * 0.15f, h * 0.3f)
                        
                        // Ogive
                        cubicTo(w * 0.15f, h * 0.15f, w * 0.4f, 0f, w / 2f, 0f)
                        
                        close()
                    }

                    // Bullet Shape Box
                    Box(
                        modifier = Modifier
                            .width(60.dp) // Reset relative width to be slimmer
                            .height(160.dp) 
                            .graphicsLayer {
                                shadowElevation = if(isActive) 20f else 0f
                                shape = bulletShape
                                clip = true
                            }
                            .background(
                                brush = if(isActive) Brush.horizontalGradient(
                                    colors = listOf(
                                        Color(0xFFB8860B), 
                                        Color(0xFFFFD700), 
                                        Color(0xFFFFFACD), 
                                        Color(0xFFB8860B)
                                    )
                                ) else Brush.verticalGradient(
                                    colors = listOf(Color.DarkGray.copy(alpha=0.3f), Color.Black.copy(alpha=0.5f))
                                ),
                                shape = bulletShape
                            )
                            .border(
                                width = if(isActive) 2.dp else 1.dp,
                                color = if(isActive) Color(0xFFFFD700).copy(alpha=0.8f) else Color.Gray.copy(alpha=0.3f),
                                shape = bulletShape
                            )
                    ) {
                        // Glossy reflection on Casing and Bullet
                        if (isActive) {
                             Canvas(modifier = Modifier.fillMaxSize()) {
                                 // Bullet Specular (Top)
                                 drawRoundRect(
                                     color = Color.White.copy(alpha = 0.6f),
                                     topLeft = Offset(size.width * 0.35f, size.height * 0.1f),
                                     size = androidx.compose.ui.geometry.Size(size.width * 0.15f, size.height * 0.15f),
                                     cornerRadius = androidx.compose.ui.geometry.CornerRadius(10f)
                                 )
                                 // Casing Specular (Body)
                                 drawRoundRect(
                                     color = Color.White.copy(alpha = 0.4f),
                                     topLeft = Offset(size.width * 0.2f, size.height * 0.5f),
                                     size = androidx.compose.ui.geometry.Size(size.width * 0.1f, size.height * 0.35f),
                                     cornerRadius = androidx.compose.ui.geometry.CornerRadius(5f)
                                 )
                             }
                        }
                    }
                }
            }

            Spacer(modifier = Modifier.height(16.dp))

            if (currentAmmo == 0) {
                Text(
                    "RELOAD REQUIRED",
                    color = Color.Red,
                    style = MaterialTheme.typography.headlineSmall,
                    fontWeight = FontWeight.Bold
                )
            } else {
                Text(
                    "$currentAmmo / $maxAmmo",
                    color = NeonAmber.copy(alpha=0.5f),
                    style = MaterialTheme.typography.titleMedium
                )
            }
        }


    }
}




@Composable
fun CyberpunkPanel(
    modifier: Modifier = Modifier,
    borderColor: Color = NeonCyan,
    backgroundColor: Color = Color(0xFF000000),
    content: @Composable () -> Unit
) {
    Box(
        modifier = modifier
            .background(backgroundColor, RoundedCornerShape(8.dp))
            .border(
                BorderStroke(1.dp, borderColor.copy(alpha = 0.6f)),
                RoundedCornerShape(8.dp)
            )
            .padding(1.dp) // Inner padding
    ) {
        // Corner Accents
        Canvas(modifier = Modifier.matchParentSize()) {
            val length = 10.dp.toPx()
            val stroke = 2.dp.toPx()
            val color = borderColor
            
            // Top Left
            drawLine(color, androidx.compose.ui.geometry.Offset(0f, 0f), androidx.compose.ui.geometry.Offset(length, 0f), stroke)
            drawLine(color, androidx.compose.ui.geometry.Offset(0f, 0f), androidx.compose.ui.geometry.Offset(0f, length), stroke)
            
            // Top Right
            drawLine(color, androidx.compose.ui.geometry.Offset(size.width, 0f), androidx.compose.ui.geometry.Offset(size.width - length, 0f), stroke)
            drawLine(color, androidx.compose.ui.geometry.Offset(size.width, 0f), androidx.compose.ui.geometry.Offset(size.width, length), stroke)
            
            // Bottom Left
            drawLine(color, androidx.compose.ui.geometry.Offset(0f, size.height), androidx.compose.ui.geometry.Offset(length, size.height), stroke)
            drawLine(color, androidx.compose.ui.geometry.Offset(0f, size.height), androidx.compose.ui.geometry.Offset(0f, size.height - length), stroke)
            
             // Bottom Right
            drawLine(color, androidx.compose.ui.geometry.Offset(size.width, size.height), androidx.compose.ui.geometry.Offset(size.width - length, size.height), stroke)
            drawLine(color, androidx.compose.ui.geometry.Offset(size.width, size.height), androidx.compose.ui.geometry.Offset(size.width, size.height - length), stroke)
        }
        content()
    }
}

@Composable
fun ArcadeButton(
    text: String,
    color: Color,
    onClick: () -> Unit,
    modifier: Modifier = Modifier.size(110.dp)
) {
    var isPressed by remember { mutableStateOf(false) }
    
    Box(
        modifier = modifier
            .clickable(
                interactionSource = remember { MutableInteractionSource() },
                indication = null
            ) { 
                 isPressed = true
                 onClick()
            }
            .graphicsLayer {
                val scale = if (isPressed) 0.95f else 1f
                scaleX = scale
                scaleY = scale
            }
    ) {
         // Button Release logic
         LaunchedEffect(isPressed) {
             if(isPressed) {
                 kotlinx.coroutines.delay(100)
                 isPressed = false
             }
         }
    
        Canvas(modifier = Modifier.fillMaxSize()) {
            val cornerRadius = androidx.compose.ui.geometry.CornerRadius(size.minDimension / 2)
            val strokeWidth = 4.dp.toPx()
            val padding = 8.dp.toPx()
            
            // Outer Ring
            drawRoundRect(
                color = color.copy(alpha = 0.3f),
                cornerRadius = cornerRadius,
                style = Stroke(width = strokeWidth)
            )
            
            // Inner Fill (Glassy)
            drawRoundRect(
                brush = Brush.horizontalGradient(
                    colors = listOf(color.copy(alpha = 0.1f), color.copy(alpha = 0.6f))
                ),
                cornerRadius = androidx.compose.ui.geometry.CornerRadius((size.minDimension - padding*2) / 2),
                topLeft = Offset(padding, padding),
                size = androidx.compose.ui.geometry.Size(size.width - padding*2, size.height - padding*2)
            )
            
            // Inner Border
            drawRoundRect(
                color = color,
                cornerRadius = androidx.compose.ui.geometry.CornerRadius((size.minDimension - padding*2) / 2),
                topLeft = Offset(padding, padding),
                size = androidx.compose.ui.geometry.Size(size.width - padding*2, size.height - padding*2),
                style = Stroke(width = 2.dp.toPx())
            )
        }
        
        Text(
            text = text,
            style = MaterialTheme.typography.titleLarge,
            color = Color.White,
            modifier = Modifier.align(Alignment.Center)
        )
    }
}

@SuppressLint("MissingPermission")
@Composable
fun DeviceListDialog(
    adapter: BluetoothAdapter?,
    onDismiss: () -> Unit,
    onDeviceSelected: (android.bluetooth.BluetoothDevice) -> Unit
) {
    if (adapter == null || !adapter.isEnabled) {
        AlertDialog(
            onDismissRequest = onDismiss,
            confirmButton = { TextButton(onClick = onDismiss) { Text("OK", color=NeonCyan) } },
            containerColor = Color(0xFF1E1E2C),
            titleContentColor = NeonCyan,
            textContentColor = Color.White,
            title = { Text("BLUETOOTH ERROR") },
            text = { Text("Bluetooth is not enabled.") }
        )
        return
    }

    val pairedDevices = try {
        adapter.bondedDevices.toList()
    } catch (e: SecurityException) {
        emptyList()
    }

    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = Color(0xFF1E1E2C), // Deep Surface
        titleContentColor = NeonCyan,
        textContentColor = Color.White,
        confirmButton = {
             TextButton(onClick = onDismiss) { Text("CANCEL", color = NeonMagenta) }
        },
        title = { Text("SELECT DEVICE", fontWeight = FontWeight.Bold) },
        text = {
            LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                 items(pairedDevices) { device ->
                     CyberpunkPanel(
                         modifier = Modifier.fillMaxWidth().clickable { onDeviceSelected(device) },
                         borderColor = NeonCyan.copy(alpha=0.5f),
                         backgroundColor = Color.Transparent
                     ) {
                         Column(modifier = Modifier.padding(16.dp)) {
                             Text(device.name ?: "Unknown", color = NeonCyan, fontWeight = FontWeight.Bold)
                             Text(device.address, color = Color.Gray, fontSize = 12.sp)
                         }
                     }
                 }
            }
        }
    )
}

@Composable
fun LogViewerDialog(
    logs: List<String>,
    onDismiss: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        confirmButton = {
            TextButton(onClick = onDismiss) { Text("CLOSE", color = NeonCyan) }
        },
        containerColor = Color(0xFF000000),
        titleContentColor = NeonCyan,
        textContentColor = NeonCyan,
        title = { Text("SYSTEM LOGS") },
        text = {
            LazyColumn(
                modifier = Modifier.heightIn(max = 400.dp).fillMaxWidth(),
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                items(logs) { log ->
                    Text(
                        text = log,
                        style = MaterialTheme.typography.bodySmall,
                        fontFamily = androidx.compose.ui.text.font.FontFamily.Monospace,
                        color = NeonCyan
                    )
                    Divider(color = NeonCyan.copy(alpha=0.2f))
                }
            }
        }
    )
}

@Preview(showBackground = true)
@Composable
fun GameScreenPreview() {
    Esdl_term_projectTheme {
        Box(modifier = Modifier.background(Color.Black)) {
            GameScreen(
                connectionState = ConnectionState.Connected("STM32-Game"),
                gameStatus = GameStatus(score = 1203, bullets = 2),
                onConnect = {},
                onDisconnect = {},
                onSendMessage = {},
                onCheckPermissions = {},
                bluetoothAdapter = null,
                messageLogs = listOf("어쩌구 저쩌구~")
            )
        }
    }
}